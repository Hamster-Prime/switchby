#include "activity/player_activity.hpp"
#include "api/emby_client.hpp"
#include <borealis/core/application.hpp>
#include <chrono>

#ifdef __SWITCH__
// Switch specific includes
#include <EGL/egl.h>
#else
#include <GLFW/glfw3.h>
#endif

// Custom view to handle MPV rendering
class MpvView : public brls::Box {
public:
    MpvView() : brls::Box(brls::Axis::COLUMN) {
        this->setDimensions(brls::Application::contentWidth, brls::Application::contentHeight);
        this->setBackgroundColor(nvgRGB(0, 0, 0));
    }

    void setMpvContext(mpv_render_context* ctx) {
        this->mpv_gl = ctx;
    }

    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override {
        // Draw video first (under UI)
        if (this->mpv_gl) {
            // We need to handle GL context here
            // In Borealis, draw is called within a frame. 
            // MPV rendering might need to be carefully placed.
            // For now, simple FBO rendering.
            
            GLint fbo = 0;
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fbo);

            int w = (int)width;
            int h = (int)height;

            mpv_opengl_fbo mpv_fbo{fbo, w, h, 0};
            int flip = 1;
            mpv_render_param params[]{
                {MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo},
                {MPV_RENDER_PARAM_FLIP_Y, &flip},
                {MPV_RENDER_PARAM_INVALID, nullptr}
            };
            
            mpv_render_context_render(this->mpv_gl, params);
        }

        // Draw children (UI overlays)
        brls::Box::draw(vg, x, y, width, height, style, ctx);
    }

private:
    mpv_render_context* mpv_gl = nullptr;
};

static void* get_proc_address(void* ctx, const char* name) {
#ifdef __SWITCH__
    return (void*)eglGetProcAddress(name);
#else
    return (void*)glfwGetProcAddress(name);
#endif
}

PlayerActivity::PlayerActivity(std::string url, std::string itemId, long long resumePositionTicks)
    : url(url), itemId(itemId), resumePositionTicks(resumePositionTicks) {
    this->lastReportTime = std::chrono::steady_clock::now();
}

PlayerActivity::~PlayerActivity() {
    if (this->hasStartedPlayback) {
        long posTicks = (long)(this->position * 10000000);
        EmbyClient::instance().reportPlaybackStopped(this->itemId, posTicks);
    }
    
    if (this->mpv_gl) {
        mpv_render_context_free(this->mpv_gl);
    }
    if (this->mpv) {
        mpv_terminate_destroy(this->mpv);
    }
}

brls::View* PlayerActivity::createContentView() {
    // Use our custom view as root
    MpvView* root = new MpvView();
    this->mpvView = root; // Store reference to pass context later

    // OSD Container (overlay)
    this->osdContainer = new brls::Box();
    this->osdContainer->setDimensions(brls::Application::contentWidth, brls::Application::contentHeight);
    this->osdContainer->setAxis(brls::Axis::COLUMN);
    this->osdContainer->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
    this->osdContainer->setPadding(40);

    // Top bar
    brls::Box* topBar = new brls::Box();
    topBar->setAxis(brls::Axis::ROW);
    
    this->lblTitle = new brls::Label();
    this->lblTitle->setText("Loading...");
    this->lblTitle->setFontSize(24);
    this->lblTitle->setTextColor(nvgRGB(255, 255, 255));
    topBar->addView(this->lblTitle);
    
    this->osdContainer->addView(topBar);

    // Center status (pause icon, buffering, etc.)
    brls::Box* centerBox = new brls::Box();
    centerBox->setJustifyContent(brls::JustifyContent::CENTER);
    centerBox->setAlignItems(brls::AlignItems::CENTER);
    centerBox->setGrow(1.0f);
    
    this->lblStatus = new brls::Label();
    this->lblStatus->setText("⏳ Buffering...");
    this->lblStatus->setFontSize(48);
    this->lblStatus->setTextColor(nvgRGB(255, 255, 255));
    centerBox->addView(this->lblStatus);
    
    this->osdContainer->addView(centerBox);

    // Bottom bar (progress)
    brls::Box* bottomBar = new brls::Box();
    bottomBar->setAxis(brls::Axis::COLUMN);
    
    // Time labels row
    brls::Box* timeRow = new brls::Box();
    timeRow->setAxis(brls::Axis::ROW);
    timeRow->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
    timeRow->setMarginBottom(10);
    
    this->lblTime = new brls::Label();
    this->lblTime->setText("00:00 / 00:00");
    this->lblTime->setFontSize(18);
    this->lblTime->setTextColor(nvgRGB(255, 255, 255));
    timeRow->addView(this->lblTime);
    
    bottomBar->addView(timeRow);
    
    // Progress bar
    this->progressBg = new brls::Box();
    this->progressBg->setHeight(8);
    this->progressBg->setGrow(1.0f);
    this->progressBg->setBackgroundColor(nvgRGBA(255, 255, 255, 80));
    this->progressBg->setCornerRadius(4);
    
    this->progressFill = new brls::Box();
    this->progressFill->setHeight(8);
    this->progressFill->setWidth(0);
    this->progressFill->setBackgroundColor(nvgRGB(0, 150, 255));
    this->progressFill->setCornerRadius(4);
    this->progressBg->addView(this->progressFill);
    
    bottomBar->addView(this->progressBg);
    
    // Controls hint
    brls::Label* hint = new brls::Label();
    hint->setText("A: Play/Pause   L/R: Seek   X: Audio   Y: Subs   B: Back");
    hint->setFontSize(16);
    hint->setTextColor(nvgRGB(170, 170, 170));
    hint->setMarginTop(15);
    bottomBar->addView(hint);
    
    this->osdContainer->addView(bottomBar);
    root->addView(this->osdContainer);

    return root;
}

void PlayerActivity::onContentAvailable() {
    this->initMpv();
    
    // Pass the created context to the view for drawing
    if (this->mpvView) {
        ((MpvView*)this->mpvView)->setMpvContext(this->mpv_gl);
    }

    // Register controls
    this->registerAction("Play/Pause", brls::ControllerButton::BUTTON_A, [this](brls::View* view) {
        this->togglePause();
        return true;
    });
    
    this->registerAction("Back", brls::ControllerButton::BUTTON_B, [this](brls::View* view) {
        brls::Application::popActivity();
        return true;
    });
    
    this->registerAction("Seek Back", brls::ControllerButton::BUTTON_LB, [this](brls::View* view) {
        this->seek(-10);
        return true;
    });
    
    this->registerAction("Seek Forward", brls::ControllerButton::BUTTON_RB, [this](brls::View* view) {
        this->seek(10);
        return true;
    });
    
    // D-pad seeking
    this->registerAction("Seek Back", brls::ControllerButton::BUTTON_LEFT, [this](brls::View* view) {
        this->seek(-5);
        return true;
    });
    
    this->registerAction("Seek Forward", brls::ControllerButton::BUTTON_RIGHT, [this](brls::View* view) {
        this->seek(5);
        return true;
    });

    this->registerAction("Cycle Audio", brls::ControllerButton::BUTTON_X, [this](brls::View* view) {
        this->cycleTrack("audio");
        return true;
    });

    this->registerAction("Cycle Subs", brls::ControllerButton::BUTTON_Y, [this](brls::View* view) {
        this->cycleTrack("sub");
        return true;
    });
}

void PlayerActivity::initMpv() {
    this->mpv = mpv_create();
    if (!this->mpv) {
        this->lblStatus->setText("❌ MPV init failed");
        return;
    }

    mpv_set_option_string(this->mpv, "terminal", "yes");
    mpv_set_option_string(this->mpv, "msg-level", "all=v");
    mpv_set_option_string(this->mpv, "vd-lavc-threads", "4");
    mpv_set_option_string(this->mpv, "hwdec", "auto");
    
    if (mpv_initialize(this->mpv) < 0) {
        this->lblStatus->setText("❌ MPV init failed");
        return;
    }

    mpv_opengl_init_params gl_init_params{get_proc_address, nullptr};
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, (void *)MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&this->mpv_gl, this->mpv, params) < 0) {
        brls::Logger::error("Failed to create mpv render context");
        this->lblStatus->setText("❌ Render context failed");
        return;
    }

    const char* cmd[] = {"loadfile", this->url.c_str(), nullptr};
    mpv_command(this->mpv, cmd);
    
    this->lblStatus->setText("⏳ Buffering...");
    
    // Start playback reporting
    EmbyClient::instance().reportPlaybackStart(this->itemId);
    this->hasStartedPlayback = true;
    this->lastReportTime = std::chrono::steady_clock::now();
}

void PlayerActivity::updateOsd() {
    if (!this->mpv) return;

    // Get position and duration
    double pos = 0, dur = 0;
    mpv_get_property(this->mpv, "time-pos", MPV_FORMAT_DOUBLE, &pos);
    mpv_get_property(this->mpv, "duration", MPV_FORMAT_DOUBLE, &dur);

    // Resume playback - seek to saved position once duration is known
    if (!this->hasSeekToResume && this->resumePositionTicks > 0 && dur > 0) {
        double resumeSeconds = (double)this->resumePositionTicks / 10000000.0;
        // Only resume if we're not too close to the end (within 95%)
        if (resumeSeconds < dur * 0.95) {
            char seekCmd[64];
            snprintf(seekCmd, sizeof(seekCmd), "%.1f", resumeSeconds);
            const char* args[] = {"seek", seekCmd, "absolute", nullptr};
            mpv_command(this->mpv, args);
            brls::Application::notify("Resuming from " + formatTime(resumeSeconds));
        }
        this->hasSeekToResume = true;
    }

    this->position = pos;
    this->duration = dur;

    // Update time label
    this->lblTime->setText(formatTime(pos) + " / " + formatTime(dur));

    // Update progress bar
    if (dur > 0) {
        float progress = pos / dur;
        float maxWidth = brls::Application::contentWidth - 80;
        this->progressFill->setWidth(maxWidth * progress);
    }

    // Check pause state
    int paused = 0;
    mpv_get_property(this->mpv, "pause", MPV_FORMAT_FLAG, &paused);
    this->isPaused = (paused != 0);

    // Check if buffering
    int buffering = 0;
    mpv_get_property(this->mpv, "paused-for-cache", MPV_FORMAT_FLAG, &buffering);

    if (buffering) {
        this->lblStatus->setText("⏳ Buffering...");
        this->lblStatus->setVisibility(brls::Visibility::VISIBLE);
    } else if (this->isPaused) {
        this->lblStatus->setText("⏸");
        this->lblStatus->setVisibility(brls::Visibility::VISIBLE);
    } else {
        this->lblStatus->setVisibility(brls::Visibility::GONE);
    }
    
    // Get media title
    char* title = nullptr;
    if (mpv_get_property(this->mpv, "media-title", MPV_FORMAT_STRING, &title) == 0 && title) {
        this->lblTitle->setText(title);
        mpv_free(title);
    }
    
    this->reportProgress();
}

void PlayerActivity::togglePause() {
    if (!this->mpv) return;
    
    int pause = this->isPaused ? 0 : 1;
    mpv_set_property(this->mpv, "pause", MPV_FORMAT_FLAG, &pause);
    
    // Immediate report on pause/resume
    long posTicks = (long)(this->position * 10000000);
    EmbyClient::instance().reportPlaybackProgress(this->itemId, posTicks, pause != 0);
}

void PlayerActivity::reportProgress() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - this->lastReportTime).count();
    
    if (elapsed >= 10) { // Report every 10 seconds
        long posTicks = (long)(this->position * 10000000);
        EmbyClient::instance().reportPlaybackProgress(this->itemId, posTicks, this->isPaused);
        this->lastReportTime = now;
    }
}

void PlayerActivity::seek(double seconds) {
    if (!this->mpv) return;
    
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "%.1f", seconds);
    const char* args[] = {"seek", cmd, "relative", nullptr};
    mpv_command(this->mpv, args);
}

std::string PlayerActivity::formatTime(double seconds) {
    if (seconds < 0) seconds = 0;
    
    int h = (int)(seconds / 3600);
    int m = (int)((seconds - h * 3600) / 60);
    int s = (int)(seconds - h * 3600 - m * 60);
    
    char buf[32];
    if (h > 0) {
        snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    } else {
        snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    }
    return buf;
}

void PlayerActivity::cycleTrack(const std::string& type) {
    if (!this->mpv) return;

    int64_t count = 0;
    mpv_get_property(this->mpv, "track-list/count", MPV_FORMAT_INT64, &count);

    std::vector<int64_t> trackIds;
    int currentIndex = -1;
    
    // For subtitles, add an "Off" option at the start
    if (type == "sub") {
        trackIds.push_back(-1); // -1 represents 'no'
        
        char* sid = nullptr;
        mpv_get_property(this->mpv, "sid", MPV_FORMAT_STRING, &sid);
        if (!sid || std::string(sid) == "no") {
            currentIndex = 0;
        }
        if (sid) mpv_free(sid);
    }

    for (int i = 0; i < count; i++) {
        char* trackType = nullptr;
        std::string path = "track-list/" + std::to_string(i) + "/type";
        mpv_get_property(this->mpv, path.c_str(), MPV_FORMAT_STRING, &trackType);
        
        if (trackType && std::string(trackType) == type) {
            int64_t id = 0;
            path = "track-list/" + std::to_string(i) + "/id";
            mpv_get_property(this->mpv, path.c_str(), MPV_FORMAT_INT64, &id);
            
            trackIds.push_back(id);
            
            int selected = 0;
            path = "track-list/" + std::to_string(i) + "/selected";
            mpv_get_property(this->mpv, path.c_str(), MPV_FORMAT_FLAG, &selected);
            
            if (selected) currentIndex = trackIds.size() - 1;
        }
        if (trackType) mpv_free(trackType);
    }
    
    if (trackIds.empty() || (type == "audio" && trackIds.size() <= 1)) {
        if (type != "sub") {
            brls::Application::notify("No extra " + type + " tracks");
            return;
        }
    }
    
    int nextIndex = (currentIndex + 1) % trackIds.size();
    int64_t nextId = trackIds[nextIndex];
    const char* prop = (type == "audio" ? "aid" : "sid");

    if (nextId == -1) {
        mpv_set_property_string(this->mpv, prop, "no");
        brls::Application::notify("Subtitles: Off");
    } else {
        mpv_set_property(this->mpv, prop, MPV_FORMAT_INT64, &nextId);
        
        // Find metadata for notification
        for (int i = 0; i < count; i++) {
            int64_t id = 0;
            std::string path = "track-list/" + std::to_string(i) + "/id";
            mpv_get_property(this->mpv, path.c_str(), MPV_FORMAT_INT64, &id);
            
            if (id == nextId) {
                char* title = nullptr;
                char* lang = nullptr;
                path = "track-list/" + std::to_string(i) + "/title";
                mpv_get_property(this->mpv, path.c_str(), MPV_FORMAT_STRING, &title);
                path = "track-list/" + std::to_string(i) + "/lang";
                mpv_get_property(this->mpv, path.c_str(), MPV_FORMAT_STRING, &lang);
                
                std::string label = (type == "audio" ? "Audio: " : "Sub: ");
                std::string t = title ? title : "";
                std::string l = lang ? lang : "";
                
                if (!t.empty()) label += t;
                if (!l.empty()) label += (t.empty() ? "" : " ") + std::string("(") + l + ")";
                if (t.empty() && l.empty()) label += "Track " + std::to_string(id);
                
                brls::Application::notify(label);
                
                if (title) mpv_free(title);
                if (lang) mpv_free(lang);
                break;
            }
        }
    }
}

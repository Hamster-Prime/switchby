#include "activity/player_activity.hpp"
#include <borealis/core/application.hpp>
#include <chrono>

#ifdef __SWITCH__
// Switch specific includes
#else
#include <GLFW/glfw3.h>
#endif

static void* get_proc_address(void* ctx, const char* name) {
#ifdef __SWITCH__
    return nullptr;
#else
    return (void*)glfwGetProcAddress(name);
#endif
}

PlayerActivity::PlayerActivity(std::string url) : url(url) {}

PlayerActivity::~PlayerActivity() {
    if (this->mpv_gl) {
        mpv_render_context_free(this->mpv_gl);
    }
    if (this->mpv) {
        mpv_terminate_destroy(this->mpv);
    }
}

brls::View* PlayerActivity::createContentView() {
    brls::Box* root = new brls::Box();
    root->setDimensions(brls::Application::contentWidth, brls::Application::contentHeight);
    root->setBackgroundColor(brls::RGB(0, 0, 0));

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
    this->lblTitle->setTextColor("#FFFFFF");
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
    this->lblStatus->setTextColor("#FFFFFF");
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
    this->lblTime->setTextColor("#FFFFFF");
    timeRow->addView(this->lblTime);
    
    bottomBar->addView(timeRow);
    
    // Progress bar
    this->progressBg = new brls::Box();
    this->progressBg->setHeight(8);
    this->progressBg->setGrow(1.0f);
    this->progressBg->setBackgroundColor(brls::RGBA(255, 255, 255, 80));
    this->progressBg->setCornerRadius(4);
    
    this->progressFill = new brls::Box();
    this->progressFill->setHeight(8);
    this->progressFill->setWidth(0);
    this->progressFill->setBackgroundColor(brls::RGB(0, 150, 255));
    this->progressFill->setCornerRadius(4);
    this->progressBg->addView(this->progressFill);
    
    bottomBar->addView(this->progressBg);
    
    // Controls hint
    brls::Label* hint = new brls::Label();
    hint->setText("A: Play/Pause   L/R: ±10s   B: Back");
    hint->setFontSize(16);
    hint->setTextColor("#AAAAAA");
    hint->setMarginTop(15);
    bottomBar->addView(hint);
    
    this->osdContainer->addView(bottomBar);
    root->addView(this->osdContainer);

    return root;
}

void PlayerActivity::onContentAvailable() {
    this->initMpv();
    
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
}

void PlayerActivity::draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) {
    if (!this->mpv_gl) {
        brls::Activity::draw(vg, x, y, width, height, style, ctx);
        return;
    }

    // Render video
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

    // Update playback state
    this->updateOsd();

    // Draw OSD overlay
    brls::Activity::draw(vg, x, y, width, height, style, ctx);
}

void PlayerActivity::updateOsd() {
    if (!this->mpv) return;

    // Get position and duration
    double pos = 0, dur = 0;
    mpv_get_property(this->mpv, "time-pos", MPV_FORMAT_DOUBLE, &pos);
    mpv_get_property(this->mpv, "duration", MPV_FORMAT_DOUBLE, &dur);
    
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
}

void PlayerActivity::togglePause() {
    if (!this->mpv) return;
    
    int pause = this->isPaused ? 0 : 1;
    mpv_set_property(this->mpv, "pause", MPV_FORMAT_FLAG, &pause);
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

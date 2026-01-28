#include "activity/player_activity.hpp"
#include <borealis/core/application.hpp>

// Helper to get OpenGL proc address
static void* get_proc_address(void* ctx, const char* name) {
    return (void*)glfwGetProcAddress(name);
}

PlayerActivity::PlayerActivity(std::string url) : url(url) {
    this->mpv = nullptr;
    this->mpv_gl = nullptr;

    // Simple OSD
    brls::Box* container = new brls::Box();
    container->setDimensions(brls::Application::contentWidth, brls::Application::contentHeight);
    
    this->osdLabel = new brls::Label();
    this->osdLabel->setText("Buffering...");
    this->osdLabel->setFontSize(24);
    this->osdLabel->setTextColor(brls::RGB(255, 255, 255));
    this->osdLabel->setMargin(50);
    
    container->addView(this->osdLabel);
    this->setContentView(container);

    // Input to close
    this->registerAction("Close", brls::ControllerButton::BUTTON_B, [this](brls::View* view) {
        brls::Application::popActivity();
        return true;
    });
}

PlayerActivity::~PlayerActivity() {
    if (this->mpv_gl) {
        mpv_render_context_free(this->mpv_gl);
    }
    if (this->mpv) {
        mpv_terminate_destroy(this->mpv);
    }
}

void PlayerActivity::initMpv() {
    this->mpv = mpv_create();
    if (!this->mpv) return;

    mpv_set_option_string(this->mpv, "terminal", "yes");
    mpv_set_option_string(this->mpv, "msg-level", "all=v");
    mpv_set_option_string(this->mpv, "vd-lavc-threads", "4");
    
    if (mpv_initialize(this->mpv) < 0) return;

    // Init GL context
    mpv_opengl_init_params gl_init_params{get_proc_address, nullptr};
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, (void *)MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&this->mpv_gl, this->mpv, params) < 0) {
        brls::Logger::error("Failed to create mpv render context");
        return;
    }

    // Play
    const char* cmd[] = {"loadfile", this->url.c_str(), nullptr};
    mpv_command(this->mpv, cmd);
}

void PlayerActivity::onContentAvailable() {
    // We defer init to here to ensure GL context is active? 
    // Actually constructor is fine in Borealis usually, but let's be safe.
    this->initMpv();
}

void PlayerActivity::draw(brls::ViewContext* context) {
    if (!this->mpv_gl) return;

    // We draw MPV as background, so we draw it BEFORE children (Activity::draw calls draw on children)
    // Actually we override draw(), so we must call Activity::draw() to draw UI on top.

    // 1. Draw Video
    GLint fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fbo);

    int w = brls::Application::contentWidth;
    int h = brls::Application::contentHeight;

    mpv_opengl_fbo mpv_fbo{static_cast<int>(fbo), w, h, 0};
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    
    mpv_render_context_render(this->mpv_gl, params);
    
    // 2. Draw UI
    brls::Activity::draw(context);
}

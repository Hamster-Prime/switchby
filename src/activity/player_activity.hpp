#pragma once
#include <borealis.hpp>
#include <mpv/client.h>
#include <mpv/render_gl.h>

class PlayerActivity : public brls::Activity
{
public:
    PlayerActivity(std::string url);
    ~PlayerActivity();

    brls::View* createContentView() override;
    void onContentAvailable() override;
    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;

private:
    std::string url;
    mpv_handle* mpv = nullptr;
    mpv_render_context* mpv_gl = nullptr;
    
    // OSD Elements
    brls::Box* osdContainer;
    brls::Label* lblTitle;
    brls::Label* lblTime;
    brls::Label* lblStatus;
    brls::Box* progressBg;
    brls::Box* progressFill;
    
    bool isPaused = false;
    bool osdVisible = true;
    double duration = 0;
    double position = 0;
    int64_t lastOsdTick = 0;
    
    void initMpv();
    void updateOsd();
    void togglePause();
    void seek(double seconds);
    std::string formatTime(double seconds);
};

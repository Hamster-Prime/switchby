#pragma once
#include <borealis.hpp>
#include <mpv/client.h>
#include <mpv/render_gl.h>

class PlayerActivity : public brls::Activity
{
public:
    PlayerActivity(std::string url);
    ~PlayerActivity();

    void onContentAvailable() override;
    void draw(brls::ViewContext* context) override; // Custom draw for video

private:
    std::string url;
    mpv_handle* mpv;
    mpv_render_context* mpv_gl;
    
    brls::Label* osdLabel;
    
    void initMpv();
};

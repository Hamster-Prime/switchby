#pragma once
#include <borealis.hpp>
#include <chrono> // Added missing header
#include <mpv/client.h>
#include <mpv/render_gl.h>

class PlayerActivity : public brls::Activity
{
public:
    PlayerActivity(std::string url, std::string itemId, long long resumePositionTicks = 0);
    ~PlayerActivity();

    brls::View* createContentView() override;
    void onContentAvailable() override;

private:
    std::string url;
    std::string itemId;
    long long resumePositionTicks = 0;  // Resume position in ticks (10,000,000 ticks = 1 second)
    bool hasSeekToResume = false;
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
    
    // Playback reporting
    std::chrono::steady_clock::time_point lastReportTime;
    bool hasStartedPlayback = false;
    
    // Tracks
    struct Track {
        int id;
        std::string type;
        std::string title;
        std::string lang;
        bool selected;
    };
    std::vector<Track> audioTracks;
    std::vector<Track> subTracks;
    
    void initMpv();
    void updateOsd();
    void togglePause();
    void seek(double seconds);
    void reportProgress();
    std::string formatTime(double seconds);
    
    void cycleTrack(const std::string& type);
    
    brls::View* mpvView = nullptr;
};

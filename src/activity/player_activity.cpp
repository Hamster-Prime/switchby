
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

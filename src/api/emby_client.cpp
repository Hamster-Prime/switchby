
void EmbyClient::getPlaybackInfo(const std::string& itemId, std::function<void(bool success, const PlaybackInfo& info)> cb) {
    // Construct device profile for Switch
    // This tells the server what we can handle natively
    json deviceProfile = {
        {"MaxStreamingBitrate", 20000000}, // 20 Mbps cap
        {"MusicStreamingTranscodingBitrate", 192000},
        {"TimelineOffsetSeconds", 5},
        {"DirectPlayProfiles", {
            {{"Container", "mp4,m4v"}, {"Type", "Video"}, {"VideoCodec", "h264"}, {"AudioCodec", "aac,mp3,opus"}},
            {{"Container", "mkv"}, {"Type", "Video"}, {"VideoCodec", "h264,vp8,vp9"}, {"AudioCodec", "aac,mp3,opus,vorbis,flac"}}
        }},
        {"TranscodingProfiles", {
            {{"Container", "ts"}, {"Type", "Video"}, {"AudioCodec", "aac"}, {"VideoCodec", "h264"}, {"Context", "Streaming"}, {"Protocol", "hls"}},
            {{"Container", "mp3"}, {"Type", "Audio"}, {"AudioCodec", "mp3"}, {"Context", "Streaming"}, {"Protocol", "http"}}
        }}
    };

    json body = {
        {"DeviceProfile", deviceProfile}
    };

    std::string endpoint = "/Items/" + itemId + "/PlaybackInfo";
    
    this->post(endpoint, body, [cb, this, itemId](bool success, const json& resp) {
        if (success) {
            PlaybackInfo info;
            try {
                info.playSessionId = resp.value("PlaySessionId", "");
                
                if (resp.contains("MediaSources") && resp["MediaSources"].is_array()) {
                    for (const auto& jSource : resp["MediaSources"]) {
                        MediaSource source;
                        source.id = jSource.value("Id", "");
                        source.container = jSource.value("Container", "");
                        source.supportsDirectPlay = jSource.value("SupportsDirectPlay", false);
                        source.supportsDirectStream = jSource.value("SupportsDirectStream", false);
                        source.supportsTranscoding = jSource.value("SupportsTranscoding", false);
                        
                        if (jSource.contains("TranscodingUrl")) {
                            source.transcodingUrl = this->baseUrl + jSource["TranscodingUrl"].get<std::string>();
                        }
                        
                        // Construct direct stream URL if possible
                        if (source.supportsDirectStream || source.supportsDirectPlay) {
                            std::string staticUrl = "/Videos/" + itemId + "/stream?static=true&MediaSourceId=" + source.id + "&PlaySessionId=" + info.playSessionId + "&api_key=" + this->accessToken;
                             source.directStreamUrl = this->baseUrl + staticUrl;
                        }

                        info.mediaSources.push_back(source);
                    }
                }
            } catch (...) {
                brls::Logger::error("Failed to parse playback info");
            }
            cb(true, info);
        } else {
            cb(false, PlaybackInfo{});
        }
    });
}

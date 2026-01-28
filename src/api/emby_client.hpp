#pragma once

#include <string>
#include <functional>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct EmbyUser {
    std::string id;
    std::string name;
    bool hasPassword;
};

class EmbyClient {
public:
    static EmbyClient& instance() {
        static EmbyClient instance;
        return instance;
    }

    void setServerUrl(const std::string& url);
    std::string getServerUrl() const;

    // Async Login (Verify server and get public info for now)
    void connect(const std::string& url, std::function<void(bool success, const std::string& error)> cb);

    // Authenticate user
    void authenticate(const std::string& username, const std::string& password, std::function<void(bool success, const std::string& token, const std::string& error)> cb);

    struct EmbyItem {
        std::string id;
        std::string name;
        std::string type;
        std::string collectionType;
        std::string primaryImageTag;
        std::string backdropImageTag;
        std::string overview;
        int productionYear = 0;
        double communityRating = 0.0;
    };

    // Get User Views (Home screen libraries)
    void getUserViews(std::function<void(bool success, const std::vector<EmbyItem>& items)> cb);

    // Get Items in a parent (Library contents)
    void getItems(const std::string& parentId, std::function<void(bool success, const std::vector<EmbyItem>& items)> cb);

    // Get specific item details
    void getItem(const std::string& itemId, std::function<void(bool success, const EmbyItem& item)> cb);

    // Get Playback Info (URL)
    std::string getPlaybackUrl(const std::string& itemId);

    // Playback Reporting
    void reportPlaybackStart(const std::string& itemId);
    void reportPlaybackProgress(const std::string& itemId, long positionTicks, bool isPaused);
    void reportPlaybackStopped(const std::string& itemId, long positionTicks);
    
    // Playback Info
    struct MediaSource {
        std::string id;
        std::string container;
        bool supportsDirectPlay = true;
        bool supportsDirectStream = true;
        bool supportsTranscoding = true;
        std::string transcodingUrl;
        std::string directStreamUrl;
    };
    
    struct PlaybackInfo {
        std::string playSessionId;
        std::vector<MediaSource> mediaSources;
    };

    void getPlaybackInfo(const std::string& itemId, std::function<void(bool success, const PlaybackInfo& info)> cb);

    // Download image to memory (or file)
    // type defaults to "Primary", index defaults to 0
    void downloadImage(const std::string& itemId, const std::string& tag, std::function<void(bool success, const std::string& path)> cb, const std::string& type = "Primary");

    std::string getUserId() const { return userId; }
    std::string getAccessToken() const { return accessToken; }

    // Set credentials for auto-login
    void setCredentials(const std::string& uid, const std::string& token) {
        this->userId = uid;
        this->accessToken = token;
    }

    // Get "Continue Watching" items
    void getResumeItems(std::function<void(bool success, const std::vector<EmbyItem>& items)> cb);

    // Search media
    void search(const std::string& query, std::function<void(bool success, const std::vector<EmbyItem>& items)> cb);

private:
    EmbyClient() = default;
    std::string baseUrl;
    std::string accessToken;
    std::string userId;
    
    // Helper to perform HTTP GET
    void get(const std::string& endpoint, std::function<void(bool success, const json& response)> cb);
    // Helper to perform HTTP POST
    void post(const std::string& endpoint, const json& body, std::function<void(bool success, const json& response)> cb);
};

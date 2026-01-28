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
    };

    // Get User Views (Home screen libraries)
    void getUserViews(std::function<void(bool success, const std::vector<EmbyItem>& items)> cb);

    // Get Items in a parent (Library contents)
    void getItems(const std::string& parentId, std::function<void(bool success, const std::vector<EmbyItem>& items)> cb);

    // Download image to memory (or file)
    // For simplicity, we'll download to a file and return the path
    void downloadImage(const std::string& itemId, const std::string& tag, std::function<void(bool success, const std::string& path)> cb);

    std::string getUserId() const { return userId; }
    std::string getAccessToken() const { return accessToken; }

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

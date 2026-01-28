#include "emby_client.hpp"
#include <curl/curl.h>
#include <borealis.hpp>
#include <thread>

// Simple write callback for CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void EmbyClient::setServerUrl(const std::string& url) {
    this->baseUrl = url;
    if (this->baseUrl.back() == '/') {
        this->baseUrl.pop_back();
    }
}

std::string EmbyClient::getServerUrl() const {
    return this->baseUrl;
}

void EmbyClient::connect(const std::string& url, std::function<void(bool success, const std::string& error)> cb) {
    this->setServerUrl(url);
    
    // Check public info to verify server
    this->get("/System/Info/Public", [cb](bool success, const json& resp) {
        if (success) {
            cb(true, "");
        } else {
            cb(false, "Failed to connect to server");
        }
    });
}

void EmbyClient::authenticate(const std::string& username, const std::string& password, std::function<void(bool success, const std::string& token, const std::string& error)> cb) {
    json body = {
        {"Username", username},
        {"Pw", password}
    };

    std::string endpoint = "/Users/AuthenticateByName";
    
    std::thread([this, endpoint, body, cb]() {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if(curl) {
            std::string url = this->baseUrl + endpoint;
            
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            std::string authHeader = "Authorization: MediaBrowser Client=\"SwitchBy\", Device=\"Nintendo Switch\", DeviceId=\"SwitchByDevice\", Version=\"0.1.0\"";
            headers = curl_slist_append(headers, authHeader.c_str());

            std::string jsonStr = body.dump();

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

            res = curl_easy_perform(curl);
            
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);

            brls::sync([res, readBuffer, http_code, cb, this]() {
                if (res != CURLE_OK) {
                    cb(false, "", "Network error");
                } else if (http_code == 200) {
                    try {
                        json j = json::parse(readBuffer);
                        this->accessToken = j["AccessToken"];
                        this->userId = j["User"]["Id"];
                        cb(true, this->accessToken, "");
                    } catch (...) {
                        cb(false, "", "Parse error");
                    }
                } else {
                    cb(false, "", "Auth failed: " + std::to_string(http_code));
                }
            });
        } else {
            brls::sync([cb]() { cb(false, "", "Init failed"); });
        }
    }).detach();
}

void EmbyClient::getUserViews(std::function<void(bool success, const std::vector<EmbyItem>& items)> cb) {
    std::string endpoint = "/Users/" + this->userId + "/Views";
    this->get(endpoint, [cb](bool success, const json& resp) {
        if (success) {
            std::vector<EmbyItem> items;
            try {
                if (resp.contains("Items") && resp["Items"].is_array()) {
                    for (const auto& jItem : resp["Items"]) {
                        EmbyItem item;
                        item.id = jItem.value("Id", "");
                        item.name = jItem.value("Name", "Unknown");
                        item.type = jItem.value("Type", "");
                        item.collectionType = jItem.value("CollectionType", "");
                        if (jItem.contains("ImageTags") && jItem["ImageTags"].contains("Primary")) {
                            item.primaryImageTag = jItem["ImageTags"]["Primary"];
                        }
                        items.push_back(item);
                    }
                }
            } catch (...) {
                brls::Logger::error("Failed to parse views");
            }
            cb(true, items);
        } else {
            cb(false, {});
        }
    });
}

void EmbyClient::get(const std::string& endpoint, std::function<void(bool success, const json& response)> cb) {
    std::thread([this, endpoint, cb]() {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if(curl) {
            std::string url = this->baseUrl + endpoint;
            
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

            struct curl_slist *headers = NULL;
            if (!this->accessToken.empty()) {
                std::string tokenHeader = "X-Emby-Token: " + this->accessToken;
                headers = curl_slist_append(headers, tokenHeader.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }

            res = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            
            curl_easy_cleanup(curl);
            if (headers) curl_slist_free_all(headers);

            brls::sync([res, readBuffer, http_code, cb]() {
                if (res == CURLE_OK && http_code == 200) {
                    try {
                        cb(true, json::parse(readBuffer));
                    } catch (...) {
                        cb(false, nullptr);
                    }
                } else {
                    brls::Logger::error("GET {} failed: {}", endpoint, http_code);
                    cb(false, nullptr);
                }
            });
        }
    }).detach();
}
void EmbyClient::getItems(const std::string& parentId, std::function<void(bool success, const std::vector<EmbyItem>& items)> cb) {
    // Basic query for items: recursive=true, fields=ImageTags
    std::string endpoint = "/Users/" + this->userId + "/Items?ParentId=" + parentId + "&Recursive=true&Fields=ImageTags,PrimaryImageAspectRatio&SortBy=SortName&SortOrder=Ascending";
    
    // For specific types, we might want to filter, but generic is fine for now
    
    this->get(endpoint, [cb](bool success, const json& resp) {
        if (success) {
            std::vector<EmbyItem> items;
            try {
                if (resp.contains("Items") && resp["Items"].is_array()) {
                    for (const auto& jItem : resp["Items"]) {
                        EmbyItem item;
                        item.id = jItem.value("Id", "");
                        item.name = jItem.value("Name", "Unknown");
                        item.type = jItem.value("Type", "");
                        if (jItem.contains("ImageTags") && jItem["ImageTags"].contains("Primary")) {
                            item.primaryImageTag = jItem["ImageTags"]["Primary"];
                        }
                        items.push_back(item);
                    }
                }
            } catch (...) {
                brls::Logger::error("Failed to parse items");
            }
            cb(true, items);
        } else {
            cb(false, {});
        }
    });
}
#include <sys/stat.h>
#include <fstream>

// ... existing code ...

// Helper to write to file
static size_t WriteToFileCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    std::ofstream* file = (std::ofstream*)userp;
    file->write((char*)contents, size * nmemb);
    return size * nmemb;
}

void EmbyClient::downloadImage(const std::string& itemId, const std::string& tag, std::function<void(bool success, const std::string& path)> cb) {
    // Determine cache path
    // Assuming we can write to ./cache for now (in switch usually sdmc:/switch/switchby/cache)
    std::string cacheDir = "cache";
    #ifdef __SWITCH__
    cacheDir = "/switch/switchby/cache";
    #endif
    
    // Create dir if not exists (simple check)
    mkdir(cacheDir.c_str(), 0777);
    
    std::string filename = cacheDir + "/" + itemId + ".jpg";
    
    // Check if file exists
    struct stat buffer;
    if (stat(filename.c_str(), &buffer) == 0) {
        cb(true, filename);
        return;
    }

    std::string endpoint = "/Items/" + itemId + "/Images/Primary?tag=" + tag + "&MaxWidth=400&Quality=90";
    
    std::thread([this, endpoint, filename, cb]() {
        CURL* curl;
        CURLcode res;

        curl = curl_easy_init();
        if(curl) {
            std::string url = this->baseUrl + endpoint;
            
            std::ofstream file(filename, std::ios::binary);
            
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToFileCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

            res = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            
            curl_easy_cleanup(curl);
            file.close();

            brls::sync([res, http_code, filename, cb]() {
                if (res == CURLE_OK && http_code == 200) {
                    cb(true, filename);
                } else {
                    // Remove failed file
                    remove(filename.c_str());
                    cb(false, "");
                }
            });
        }
    }).detach();
}

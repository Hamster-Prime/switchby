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

void EmbyClient::downloadImage(const std::string& itemId, const std::string& tag, std::function<void(bool success, const std::string& path)> cb, const std::string& type) {
    // Determine cache path
    std::string cacheDir = "cache";
    #ifdef __SWITCH__
    cacheDir = "/switch/switchby/cache";
    #endif
    
    mkdir(cacheDir.c_str(), 0777);
    
    std::string filename = cacheDir + "/" + itemId + "_" + type + ".jpg";
    
    // Check if file exists
    struct stat buffer;
    if (stat(filename.c_str(), &buffer) == 0) {
        cb(true, filename);
        return;
    }

    // Adjust max width based on type
    int maxWidth = (type == "Backdrop") ? 1280 : 400;
    std::string endpoint = "/Items/" + itemId + "/Images/" + type + "?tag=" + tag + "&MaxWidth=" + std::to_string(maxWidth) + "&Quality=90";
    
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
void EmbyClient::getItem(const std::string& itemId, std::function<void(bool success, const EmbyItem& item)> cb) {
    std::string endpoint = "/Users/" + this->userId + "/Items/" + itemId;
    
    this->get(endpoint, [cb](bool success, const json& jItem) {
        if (success) {
            EmbyItem item;
            try {
                item.id = jItem.value("Id", "");
                item.name = jItem.value("Name", "Unknown");
                item.type = jItem.value("Type", "");
                item.overview = jItem.value("Overview", "");
                item.productionYear = jItem.value("ProductionYear", 0);
                item.communityRating = jItem.value("CommunityRating", 0.0);
                
                if (jItem.contains("ImageTags")) {
                    if (jItem["ImageTags"].contains("Primary")) {
                        item.primaryImageTag = jItem["ImageTags"]["Primary"];
                    }
                    // For backdrops, it's usually an array "Backdrops" which is a list of tags.
                    // But typically in item details "BackdropImageTags" is a list.
                    if (jItem.contains("BackdropImageTags") && jItem["BackdropImageTags"].is_array() && !jItem["BackdropImageTags"].empty()) {
                         item.backdropImageTag = jItem["BackdropImageTags"][0];
                    }
                }
            } catch (...) {
                brls::Logger::error("Failed to parse item details");
            }
            cb(true, item);
        } else {
            cb(false, EmbyItem{});
        }
    });
}
std::string EmbyClient::getPlaybackUrl(const std::string& itemId) {
    // Basic direct stream URL
    // In reality, we should check PlaybackInfo first to decide transcode vs direct
    // But for MVP mpv usually handles direct streams well
    return this->baseUrl + "/Videos/" + itemId + "/stream?static=true&MediaSourceId=" + itemId + "&PlaySessionId=switchby-" + itemId + "&api_key=" + this->accessToken;
}

void EmbyClient::getResumeItems(std::function<void(bool success, const std::vector<EmbyItem>& items)> cb) {
    std::string endpoint = "/Users/" + this->userId + "/Items/Resume?Limit=12&Fields=PrimaryImageAspectRatio,ImageTags&MediaTypes=Video";
    
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
                brls::Logger::error("Failed to parse resume items");
            }
            cb(true, items);
        } else {
            cb(false, {});
        }
    });
}

void EmbyClient::search(const std::string& query, std::function<void(bool success, const std::vector<EmbyItem>& items)> cb) {
    // URL encode query (simple version)
    std::string encoded;
    for (char c : query) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.') {
            encoded += c;
        } else if (c == ' ') {
            encoded += "%20";
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            encoded += buf;
        }
    }
    
    std::string endpoint = "/Users/" + this->userId + "/Items?SearchTerm=" + encoded + "&Recursive=true&Fields=ImageTags,PrimaryImageAspectRatio&Limit=50&IncludeItemTypes=Movie,Series,Episode";
    
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
                        item.productionYear = jItem.value("ProductionYear", 0);
                        if (jItem.contains("ImageTags") && jItem["ImageTags"].contains("Primary")) {
                            item.primaryImageTag = jItem["ImageTags"]["Primary"];
                        }
                        items.push_back(item);
                    }
                }
            } catch (...) {
                brls::Logger::error("Failed to parse search results");
            }
            cb(true, items);
        } else {
            cb(false, {});
        }
    });
}

// Helper to perform HTTP POST
void EmbyClient::post(const std::string& endpoint, const json& body, std::function<void(bool success, const json& response)> cb) {
    std::thread([this, endpoint, body, cb]() {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if(curl) {
            std::string url = this->baseUrl + endpoint;
            
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            if (!this->accessToken.empty()) {
                std::string tokenHeader = "X-Emby-Token: " + this->accessToken;
                headers = curl_slist_append(headers, tokenHeader.c_str());
            }
            // Add standard client headers
            headers = curl_slist_append(headers, "X-Emby-Client: SwitchBy");
            headers = curl_slist_append(headers, "X-Emby-Device-Name: Nintendo Switch");
            headers = curl_slist_append(headers, "X-Emby-Device-Id: SwitchByDevice");
            headers = curl_slist_append(headers, "X-Emby-Client-Version: 0.1.0");

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

            brls::sync([res, readBuffer, http_code, cb]() {
                if (res == CURLE_OK && (http_code == 200 || http_code == 204)) {
                    try {
                        if (!readBuffer.empty())
                            cb(true, json::parse(readBuffer));
                        else
                            cb(true, json());
                    } catch (...) {
                        cb(false, nullptr);
                    }
                } else {
                    brls::Logger::error("POST {} failed: {}", endpoint, http_code);
                    cb(false, nullptr);
                }
            });
        }
    }).detach();
}

void EmbyClient::reportPlaybackStart(const std::string& itemId) {
    json body = {
        {"ItemId", itemId},
        {"PlaySessionId", "switchby-" + itemId}
    };
    this->post("/Sessions/Playing", body, [](bool, const json&){});
}

void EmbyClient::reportPlaybackProgress(const std::string& itemId, long positionTicks, bool isPaused) {
    json body = {
        {"ItemId", itemId},
        {"PlaySessionId", "switchby-" + itemId},
        {"PositionTicks", positionTicks},
        {"EventName", isPaused ? "Pause" : "TimeUpdate"},
        {"IsPaused", isPaused}
    };
    this->post("/Sessions/Playing/Progress", body, [](bool, const json&){});
}

void EmbyClient::reportPlaybackStopped(const std::string& itemId, long positionTicks) {
    json body = {
        {"ItemId", itemId},
        {"PlaySessionId", "switchby-" + itemId},
        {"PositionTicks", positionTicks}
    };
    this->post("/Sessions/Playing/Stopped", body, [](bool, const json&){});
}

void EmbyClient::getPlaybackInfo(const std::string& itemId, std::function<void(bool success, const PlaybackInfo& info)> cb) {
    // Construct device profile for Switch
    // We report broad compatibility because we use MPV which supports software decoding (local transcoding)
    // This is crucial for servers that disable server-side transcoding.
    json deviceProfile = {
        {"MaxStreamingBitrate", 120000000}, // 120 Mbps (effectively unlimited)
        {"MusicStreamingTranscodingBitrate", 320000},
        {"TimelineOffsetSeconds", 5},
        {"DirectPlayProfiles", {
            {
                {"Container", "mp4,m4v,mkv,avi,mov,wmv,flv,webm,ts,mpeg,mpg,m2ts"}, 
                {"Type", "Video"}, 
                {"VideoCodec", "h264,h265,hevc,vp8,vp9,av1,mpeg2video,mpeg4,vc1"}, 
                {"AudioCodec", "aac,mp3,opus,vorbis,flac,ac3,eac3,dts,truehd,dts-hd,pcm_s16le,pcm_s24le"}
            },
            {
                {"Container", "mp3,flac,aac,ogg,m4a,wav,wma"}, 
                {"Type", "Audio"}
            }
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

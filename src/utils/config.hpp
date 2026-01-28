#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sys/stat.h>

using json = nlohmann::json;

struct SavedServer {
    std::string url;
    std::string name;
    std::string userId;
    std::string accessToken;
    std::string username;
};

class Config {
public:
    static Config& instance() {
        static Config instance;
        return instance;
    }

    void load() {
        std::ifstream file(getConfigPath());
        if (file.is_open()) {
            try {
                json j;
                file >> j;
                
                if (j.contains("servers") && j["servers"].is_array()) {
                    servers.clear();
                    for (const auto& s : j["servers"]) {
                        SavedServer server;
                        server.url = s.value("url", "");
                        server.name = s.value("name", "");
                        server.userId = s.value("userId", "");
                        server.accessToken = s.value("accessToken", "");
                        server.username = s.value("username", "");
                        servers.push_back(server);
                    }
                }
                
                lastServerIndex = j.value("lastServerIndex", -1);
            } catch (...) {
                // Corrupted config, ignore
            }
        }
    }

    void save() {
        ensureConfigDir();
        
        json j;
        j["lastServerIndex"] = lastServerIndex;
        
        json serversJson = json::array();
        for (const auto& s : servers) {
            serversJson.push_back({
                {"url", s.url},
                {"name", s.name},
                {"userId", s.userId},
                {"accessToken", s.accessToken},
                {"username", s.username}
            });
        }
        j["servers"] = serversJson;
        
        std::ofstream file(getConfigPath());
        if (file.is_open()) {
            file << j.dump(2);
        }
    }

    void addServer(const SavedServer& server) {
        // Check if URL already exists, update if so
        for (auto& s : servers) {
            if (s.url == server.url) {
                s = server;
                save();
                return;
            }
        }
        servers.push_back(server);
        lastServerIndex = servers.size() - 1;
        save();
    }

    void removeServer(size_t index) {
        if (index < servers.size()) {
            servers.erase(servers.begin() + index);
            if (lastServerIndex >= (int)servers.size()) {
                lastServerIndex = servers.empty() ? -1 : 0;
            }
            save();
        }
    }

    std::vector<SavedServer>& getServers() { return servers; }
    int getLastServerIndex() const { return lastServerIndex; }
    void setLastServerIndex(int idx) { lastServerIndex = idx; save(); }

    SavedServer* getLastServer() {
        if (lastServerIndex >= 0 && lastServerIndex < (int)servers.size()) {
            return &servers[lastServerIndex];
        }
        return nullptr;
    }

private:
    Config() { load(); }
    
    std::vector<SavedServer> servers;
    int lastServerIndex = -1;

    std::string getConfigDir() {
        #ifdef __SWITCH__
        return "/switch/switchby";
        #else
        const char* home = getenv("HOME");
        return std::string(home ? home : ".") + "/.config/switchby";
        #endif
    }

    std::string getConfigPath() {
        return getConfigDir() + "/config.json";
    }

    void ensureConfigDir() {
        std::string dir = getConfigDir();
        mkdir(dir.c_str(), 0755);
    }
};

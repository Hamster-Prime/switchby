#pragma once
#include <borealis.hpp>
#include "utils/config.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

class SettingsTab : public brls::Box
{
public:
    SettingsTab()
    {
        this->setAxis(brls::Axis::COLUMN);
        this->setPadding(30);

        // Title
        brls::Label* title = new brls::Label();
        title->setText("Settings");
        title->setFontSize(28);
        title->setMarginBottom(30);
        this->addView(title);

        // Server section
        brls::Label* serverSection = new brls::Label();
        serverSection->setText("Server");
        serverSection->setFontSize(20);
        serverSection->setTextColor(nvgRGB(136, 136, 136));
        serverSection->setMarginBottom(10);
        this->addView(serverSection);

        brls::Button* btnLogout = new brls::Button();
        btnLogout->setText("🚪 Logout / Switch Server");
        btnLogout->setWidth(400);
        btnLogout->setMarginBottom(20);
        btnLogout->registerClickAction([](brls::View* view) {
            // Clear current session
            auto& config = Config::instance();
            auto* server = config.getLastServer();
            if (server) {
                server->accessToken = "";
                server->userId = "";
                config.save();
            }
            
            brls::Application::notify("Logged out");
            // Pop to root and restart - Manual pop loop is risky without stack access.
            // Just push ServerSelect?
            // Or use exit to force restart?
            // Let's try popping once.
            brls::Application::popActivity();
            return true;
        });
        this->addView(btnLogout);

        // Cache section
        brls::Label* cacheSection = new brls::Label();
        cacheSection->setText("Cache");
        cacheSection->setFontSize(20);
        cacheSection->setTextColor(nvgRGB(136, 136, 136));
        cacheSection->setMarginTop(20);
        cacheSection->setMarginBottom(10);
        this->addView(cacheSection);

        this->lblCacheSize = new brls::Label();
        this->lblCacheSize->setText("Calculating cache size...");
        this->lblCacheSize->setMarginBottom(10);
        this->addView(this->lblCacheSize);

        brls::Button* btnClearCache = new brls::Button();
        btnClearCache->setText("🗑️ Clear Image Cache");
        btnClearCache->setWidth(400);
        btnClearCache->setMarginBottom(20);
        btnClearCache->registerClickAction([this](brls::View* view) {
            this->clearCache();
            return true;
        });
        this->addView(btnClearCache);

        // About section
        brls::Label* aboutSection = new brls::Label();
        aboutSection->setText("About");
        aboutSection->setFontSize(20);
        aboutSection->setTextColor(nvgRGB(136, 136, 136));
        aboutSection->setMarginTop(20);
        aboutSection->setMarginBottom(10);
        this->addView(aboutSection);

        brls::Label* version = new brls::Label();
        version->setText("SwitchBy v0.1.0");
        version->setMarginBottom(5);
        this->addView(version);

        brls::Label* author = new brls::Label();
        author->setText("by Hamster-Prime");
        author->setTextColor(nvgRGB(136, 136, 136));
        author->setMarginBottom(5);
        this->addView(author);

        brls::Label* credits = new brls::Label();
        credits->setText("Built with Borealis • Inspired by wiliwili");
        credits->setTextColor(nvgRGB(102, 102, 102));
        credits->setFontSize(16);
        this->addView(credits);

        // Calculate cache size
        this->updateCacheSize();
    }

private:
    brls::Label* lblCacheSize;

    std::string getCacheDir() {
        #ifdef __SWITCH__
        return "/switch/switchby/cache";
        #else
        return "cache";
        #endif
    }

    void updateCacheSize() {
        std::string dir = getCacheDir();
        size_t totalSize = 0;
        int fileCount = 0;

        DIR* d = opendir(dir.c_str());
        if (d) {
            struct dirent* entry;
            while ((entry = readdir(d)) != nullptr) {
                if (entry->d_type == DT_REG) {
                    std::string path = dir + "/" + entry->d_name;
                    struct stat st;
                    if (stat(path.c_str(), &st) == 0) {
                        totalSize += st.st_size;
                        fileCount++;
                    }
                }
            }
            closedir(d);
        }

        double sizeMB = totalSize / (1024.0 * 1024.0);
        char buf[64];
        snprintf(buf, sizeof(buf), "Cache: %.2f MB (%d files)", sizeMB, fileCount);
        this->lblCacheSize->setText(buf);
    }

    void clearCache() {
        std::string dir = getCacheDir();
        int removed = 0;

        DIR* d = opendir(dir.c_str());
        if (d) {
            struct dirent* entry;
            while ((entry = readdir(d)) != nullptr) {
                if (entry->d_type == DT_REG) {
                    std::string path = dir + "/" + entry->d_name;
                    if (remove(path.c_str()) == 0) {
                        removed++;
                    }
                }
            }
            closedir(d);
        }

        brls::Application::notify("Cleared " + std::to_string(removed) + " cached files");
        this->updateCacheSize();
    }
};

#pragma once
#include <borealis.hpp>
#include "activity/login_activity.hpp"
#include "activity/tab_activity.hpp"
#include "api/emby_client.hpp"
#include "utils/config.hpp"

class ServerSelectActivity : public brls::Activity
{
public:
    brls::View* createContentView() override
    {
        brls::Box* root = new brls::Box();
        root->setAxis(brls::Axis::COLUMN);
        root->setJustifyContent(brls::JustifyContent::CENTER);
        root->setAlignItems(brls::AlignItems::CENTER);
        root->setPadding(40);

        // Title
        brls::Label* title = new brls::Label();
        title->setText("SwitchBy");
        title->setFontSize(42);
        title->setMarginBottom(10);
        root->addView(title);

        brls::Label* subtitle = new brls::Label();
        subtitle->setText("Emby Client for Switch");
        subtitle->setFontSize(20);
        subtitle->setTextColor("#888888");
        subtitle->setMarginBottom(40);
        root->addView(subtitle);

        // Saved servers container
        this->serversBox = new brls::Box();
        this->serversBox->setAxis(brls::Axis::COLUMN);
        this->serversBox->setAlignItems(brls::AlignItems::CENTER);
        this->serversBox->setMarginBottom(30);
        root->addView(this->serversBox);

        // Add new server button
        brls::Button* btnAdd = new brls::Button();
        btnAdd->setLabel("+ Add New Server");
        btnAdd->setStyle(brls::ButtonStyle::BORDERED);
        btnAdd->setWidth(350);
        btnAdd->registerClickAction([this](brls::View* view) {
            this->promptNewServer();
            return true;
        });
        root->addView(btnAdd);

        return root;
    }

    void onContentAvailable() override
    {
        this->refreshServerList();
        
        // Auto-login if we have a saved session
        auto* lastServer = Config::instance().getLastServer();
        if (lastServer && !lastServer->accessToken.empty()) {
            brls::Application::notify("Reconnecting to " + lastServer->name + "...");
            
            EmbyClient::instance().setServerUrl(lastServer->url);
            EmbyClient::instance().setCredentials(lastServer->userId, lastServer->accessToken);
            
            // Verify token still valid
            EmbyClient::instance().getUserViews([this, lastServer](bool success, const std::vector<EmbyClient::EmbyItem>& items) {
                if (success) {
                    brls::Application::notify("Welcome back, " + lastServer->username + "!");
                    brls::Application::pushActivity(new TabActivity(), brls::TransitionAnimation::FADE);
                } else {
                    brls::Application::notify("Session expired, please login again");
                }
            });
        }
    }

private:
    brls::Box* serversBox;

    void refreshServerList() {
        this->serversBox->clearViews();
        
        auto& servers = Config::instance().getServers();
        
        if (servers.empty()) {
            brls::Label* noServers = new brls::Label();
            noServers->setText("No saved servers");
            noServers->setTextColor("#666666");
            noServers->setMarginBottom(20);
            this->serversBox->addView(noServers);
            return;
        }

        for (size_t i = 0; i < servers.size(); i++) {
            const auto& server = servers[i];
            
            brls::Box* row = new brls::Box();
            row->setAxis(brls::Axis::ROW);
            row->setAlignItems(brls::AlignItems::CENTER);
            row->setMarginBottom(10);

            brls::Button* btn = new brls::Button();
            std::string label = server.name.empty() ? server.url : server.name;
            if (!server.username.empty()) {
                label += " (" + server.username + ")";
            }
            btn->setLabel(label);
            btn->setWidth(300);
            btn->registerClickAction([this, i](brls::View* view) {
                this->connectToServer(i);
                return true;
            });
            row->addView(btn);

            // Delete button
            brls::Button* btnDel = new brls::Button();
            btnDel->setLabel("✕");
            btnDel->setStyle(brls::ButtonStyle::BORDERLESS);
            btnDel->setWidth(50);
            btnDel->registerClickAction([this, i](brls::View* view) {
                Config::instance().removeServer(i);
                this->refreshServerList();
                return true;
            });
            row->addView(btnDel);

            this->serversBox->addView(row);
        }
    }

    void promptNewServer() {
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this](std::string text) {
                if (text.empty()) return;
                
                brls::Application::notify("Connecting...");
                
                EmbyClient::instance().connect(text, [this, text](bool success, const std::string& error) {
                    if (success) {
                        brls::Application::pushActivity(new LoginActivity(text, true), brls::TransitionAnimation::SLIDE_LEFT);
                    } else {
                        brls::Application::notify("Error: " + error);
                    }
                });
            },
            "Enter Emby Server URL",
            "http://",
            100
        );
    }

    void connectToServer(size_t index) {
        auto& servers = Config::instance().getServers();
        if (index >= servers.size()) return;
        
        const auto& server = servers[index];
        Config::instance().setLastServerIndex(index);
        
        brls::Application::notify("Connecting...");
        
        EmbyClient::instance().connect(server.url, [this, index, server](bool success, const std::string& error) {
            if (success) {
                if (!server.accessToken.empty()) {
                    // Try auto-login with saved token
                    EmbyClient::instance().setCredentials(server.userId, server.accessToken);
                    EmbyClient::instance().getUserViews([this, server](bool ok, const std::vector<EmbyClient::EmbyItem>& items) {
                        if (ok) {
                            brls::Application::notify("Welcome back!");
                            brls::Application::pushActivity(new TabActivity(), brls::TransitionAnimation::FADE);
                        } else {
                            // Token expired, go to login
                            brls::Application::pushActivity(new LoginActivity(server.url, false), brls::TransitionAnimation::SLIDE_LEFT);
                        }
                    });
                } else {
                    brls::Application::pushActivity(new LoginActivity(server.url, false), brls::TransitionAnimation::SLIDE_LEFT);
                }
            } else {
                brls::Application::notify("Error: " + error);
            }
        });
    }
};

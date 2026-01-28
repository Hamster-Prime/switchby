#pragma once
#include <borealis.hpp>
#include "activity/tab_activity.hpp"
#include "api/emby_client.hpp"
#include "utils/config.hpp"

class LoginActivity : public brls::Activity
{
public:
    LoginActivity(std::string serverUrl, bool isNewServer = false) 
        : serverUrl(serverUrl), isNewServer(isNewServer) {}

    brls::View* createContentView() override
    {
        brls::Box* root = new brls::Box();
        root->setAxis(brls::Axis::COLUMN);
        root->setJustifyContent(brls::JustifyContent::CENTER);
        root->setAlignItems(brls::AlignItems::CENTER);

        brls::Label* title = new brls::Label();
        title->setText("Login");
        title->setFontSize(32);
        title->setMarginBottom(20);
        root->addView(title);

        this->lblServerName = new brls::Label();
        this->lblServerName->setText(this->serverUrl);
        this->lblServerName->setFontSize(18);
        this->lblServerName->setTextColor("#888888");
        this->lblServerName->setMarginBottom(40);
        root->addView(this->lblServerName);

        this->btnUsername = new brls::Button();
        this->btnUsername->setLabel("Username");
        this->btnUsername->setWidth(300);
        this->btnUsername->setMarginBottom(10);
        root->addView(this->btnUsername);

        this->btnPassword = new brls::Button();
        this->btnPassword->setLabel("Password");
        this->btnPassword->setWidth(300);
        this->btnPassword->setMarginBottom(30);
        root->addView(this->btnPassword);

        this->btnLogin = new brls::Button();
        this->btnLogin->setLabel("Sign In");
        this->btnLogin->setStyle(brls::ButtonStyle::PRIMARY);
        this->btnLogin->setWidth(300);
        root->addView(btnLogin);

        return root;
    }

    void onContentAvailable() override
    {
        this->btnUsername->registerClickAction([this](brls::View* view) {
            brls::Application::getPlatform()->getImeManager()->openForText(
                [this](std::string text) {
                    this->username = text;
                    this->btnUsername->setLabel(text.empty() ? "Username" : text);
                },
                "Username", "", 50
            );
            return true;
        });

        this->btnPassword->registerClickAction([this](brls::View* view) {
            brls::Application::getPlatform()->getImeManager()->openForText(
                [this](std::string text) {
                    this->password = text;
                    this->btnPassword->setLabel(text.empty() ? "Password" : "••••••");
                },
                "Password", "", 50
            );
            return true;
        });

        this->btnLogin->registerClickAction([this](brls::View* view) {
            if (this->username.empty()) {
                brls::Application::notify("Please enter username");
                return true;
            }

            brls::Application::notify("Signing in...");
            EmbyClient::instance().authenticate(this->username, this->password, 
                [this](bool success, const std::string& token, const std::string& error) {
                    if (success) {
                        // Save credentials
                        SavedServer server;
                        server.url = this->serverUrl;
                        server.name = this->serverUrl; // Could fetch server name from API
                        server.userId = EmbyClient::instance().getUserId();
                        server.accessToken = token;
                        server.username = this->username;
                        
                        Config::instance().addServer(server);
                        
                        brls::Application::notify("Login Successful!");
                        brls::Application::pushActivity(new TabActivity(), brls::TransitionAnimation::FADE);
                    } else {
                        brls::Application::notify("Login Failed: " + error);
                    }
                }
            );
            return true;
        });
    }

private:
    brls::Label* lblServerName;
    brls::Button* btnUsername;
    brls::Button* btnPassword;
    brls::Button* btnLogin;

    std::string serverUrl;
    bool isNewServer;
    std::string username;
    std::string password;
};

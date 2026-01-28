#pragma once
#include <borealis.hpp>
#include "activity/tab_activity.hpp"
#include "api/emby_client.hpp"

class LoginActivity : public brls::Activity
{
public:
    CONTENT_FROM_XML_RES("activity/login.xml");

    LoginActivity(std::string serverName) {
        this->serverName = serverName;
    }

    void onContentAvailable() override
    {
        this->lbl_server_name->setText(this->serverName);

        this->btn_username->registerClickAction([this](brls::View* view) {
            brls::Application::getPlatform()->getImeManager()->openForText(
                [this](std::string text) {
                    this->username = text;
                    this->btn_username->setText(text.empty() ? "Username" : text);
                },
                "Username", "", 20
            );
            return true;
        });

        this->btn_password->registerClickAction([this](brls::View* view) {
            brls::Application::getPlatform()->getImeManager()->openForText(
                [this](std::string text) {
                    this->password = text;
                    this->btn_password->setText(text.empty() ? "Password" : "******");
                },
                "Password", "", 20, brls::ImeInputMode::PASSWORD
            );
            return true;
        });

        this->btn_login->registerClickAction([this](brls::View* view) {
            if (this->username.empty()) {
                brls::Application::notify("Please enter username");
                return true;
            }

            brls::Application::notify("Signing in...");
            EmbyClient::instance().authenticate(this->username, this->password, 
                [this](bool success, const std::string& token, const std::string& error) {
                    if (success) {
                        brls::Application::notify("Login Successful!");
                        // Go to Main Tabs
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
    BRLS_BIND(brls::Label, lbl_server_name, "lbl_server_name");
    BRLS_BIND(brls::Button, btn_username, "btn_username");
    BRLS_BIND(brls::Button, btn_password, "btn_password");
    BRLS_BIND(brls::Button, btn_login, "btn_login");

    std::string serverName;
    std::string username;
    std::string password;
};

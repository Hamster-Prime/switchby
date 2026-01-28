#pragma once
#include <borealis.hpp>
#include "activity/login_activity.hpp"
#include "api/emby_client.hpp"

class ServerSelectActivity : public brls::Activity
{
public:
    CONTENT_FROM_XML_RES("activity/server_select.xml");

    void onContentAvailable() override
    {
        this->btn_connect->registerClickAction([this](brls::View* view) {
            brls::Application::getPlatform()->getImeManager()->openForText(
                [this](std::string text) {
                    if (text.empty()) return;
                    
                    brls::Logger::info("Connecting to: {}", text);
                    brls::Application::notify("Connecting...");
                    
                    EmbyClient::instance().connect(text, [this, text](bool success, const std::string& error) {
                        if (success) {
                             // Go to Login
                             brls::Application::pushActivity(new LoginActivity(text), brls::TransitionAnimation::SLIDE_LEFT);
                        } else {
                            brls::Application::notify("Error: " + error);
                        }
                    });
                },
                "Enter Emby Server URL",
                "http://",
                100
            );
            return true;
        });
    }

private:
    BRLS_BIND(brls::Button, btn_connect, "btn_connect");
};

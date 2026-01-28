#pragma once
#include <borealis.hpp>
#include "activity/home_tab.hpp"
#include "activity/search_activity.hpp"
#include "activity/settings_activity.hpp"

class TabActivity : public brls::Activity
{
public:
    TabActivity()
    {
        brls::TabFrame* tabFrame = new brls::TabFrame();
        tabFrame->setTitle("SwitchBy");
        
        // Tab 1: Home
        tabFrame->addTab("🏠 Home", new HomeTab());
        
        // Tab 2: Search
        brls::Box* searchTab = new brls::Box();
        searchTab->setJustifyContent(brls::JustifyContent::CENTER);
        searchTab->setAlignItems(brls::AlignItems::CENTER);
        
        brls::Button* btnSearch = new brls::Button();
        btnSearch->setLabel("🔍 Open Search");
        btnSearch->setStyle(brls::ButtonStyle::PRIMARY);
        btnSearch->registerClickAction([](brls::View* view) {
            brls::Application::pushActivity(new SearchActivity());
            return true;
        });
        searchTab->addView(btnSearch);
        tabFrame->addTab("🔍 Search", searchTab);

        // Separator
        tabFrame->addSeparator();

        // Tab 3: Settings
        tabFrame->addTab("⚙️ Settings", new SettingsTab());

        this->setContentView(tabFrame);
    }
};

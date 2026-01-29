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
        // tabFrame->setTitle("SwitchBy"); // Not supported in wiliwili branch
        
        // Tab 1: Home
        // addTab expects a TabViewCreator (lambda returning View*)
        tabFrame->addTab("🏠 Home", []() -> brls::View* { return new HomeTab(); });
        
        // Tab 2: Search
        tabFrame->addTab("🔍 Search", []() -> brls::View* { return new SearchTab(); });

        // Separator
        tabFrame->addSeparator();

        // Tab 3: Settings
        tabFrame->addTab("⚙️ Settings", []() -> brls::View* { return new SettingsTab(); });

        this->setContentView(tabFrame);
    }
};

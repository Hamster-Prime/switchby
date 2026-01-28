#pragma once
#include <borealis.hpp>
#include "activity/home_tab.hpp"

class TabActivity : public brls::Activity
{
public:
    TabActivity()
    {
        brls::TabFrame* tabFrame = new brls::TabFrame();
        tabFrame->setTitle("SwitchBy");
        
        // Tab 1: Home (Now uses our dynamic HomeTab class)
        tabFrame->addTab("Home", new HomeTab());
        
        // Tab 2: Library
        brls::Box* libraryView = new brls::Box();
        libraryView->setJustifyContent(brls::JustifyContent::CENTER);
        libraryView->setAlignItems(brls::AlignItems::CENTER);
        libraryView->addView(new brls::Label("My Media"));
        tabFrame->addTab("Library", libraryView);

        // Tab 3: Collections
        brls::Box* collectionsView = new brls::Box();
        collectionsView->setJustifyContent(brls::JustifyContent::CENTER);
        collectionsView->setAlignItems(brls::AlignItems::CENTER);
        collectionsView->addView(new brls::Label("Collections"));
        tabFrame->addTab("Collections", collectionsView);

        // Separator
        tabFrame->addSeparator();

        // Tab 4: Settings
        brls::Box* settingsView = new brls::Box();
        settingsView->setJustifyContent(brls::JustifyContent::CENTER);
        settingsView->setAlignItems(brls::AlignItems::CENTER);
        settingsView->addView(new brls::Label("Settings"));
        tabFrame->addTab("Settings", settingsView);

        this->setContentView(tabFrame);
    }
};

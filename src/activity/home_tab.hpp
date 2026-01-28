#pragma once
#include <borealis.hpp>
#include "api/emby_client.hpp"

#include "activity/library_activity.hpp"

class HomeTab : public brls::Box
{
public:
    HomeTab()
    {
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setPadding(20);

        // Loading label initially
        this->label = new brls::Label();
        this->label->setText("Loading libraries...");
        this->addView(this->label);

        // Container for items
        this->container = new brls::Box();
        this->container->setDirection(brls::Direction::LEFT_TO_RIGHT);
        this->container->setWrap(true); // Allow items to wrap to next line
        this->container->setJustifyContent(brls::JustifyContent::FLEX_START);
        this->addView(this->container);
        
        // Fetch data
        brls::Application::notify("Fetching views...");
        EmbyClient::instance().getUserViews([this](bool success, const std::vector<EmbyClient::EmbyItem>& items) {
            if (success) {
                this->label->setVisibility(brls::Visibility::GONE);
                
                if (items.empty()) {
                    this->label->setText("No libraries found.");
                    this->label->setVisibility(brls::Visibility::VISIBLE);
                    return;
                }

                for (const auto& item : items) {
                    brls::Button* card = new brls::Button();
                    card->setLabel(item.name);
                    card->setMargin(10);
                    card->setWidth(200);
                    card->setHeight(100);
                    
                    card->registerClickAction([item](brls::View* view) {
                        brls::Application::pushActivity(new LibraryActivity(item.name, item.id));
                        return true;
                    });
                    
                    this->container->addView(card);
                }
                // Request focus for the first item
                // this->container->getChildren()[0]->setFocus(); 
            } else {
                this->label->setText("Failed to load libraries.");
            }
        });
    }

private:
    brls::Label* label;
    brls::Box* container;
};

#pragma once
#include <borealis.hpp>
#include "api/emby_client.hpp"
#include "activity/library_activity.hpp"
#include "view/poster_cell.hpp"

class HomeTab : public brls::Box
{
public:
    HomeTab()
    {
        this->setAxis(brls::Axis::COLUMN);
        this->setPadding(20);
        this->setGrow(1.0f);

        // Continue Watching Section
        this->resumeSection = new brls::Box();
        this->resumeSection->setAxis(brls::Axis::COLUMN);
        this->resumeSection->setMarginBottom(30);
        
        brls::Label* resumeTitle = new brls::Label();
        resumeTitle->setText("▶ Continue Watching");
        resumeTitle->setFontSize(24);
        resumeTitle->setMarginBottom(15);
        this->resumeSection->addView(resumeTitle);
        
        this->resumeContainer = new brls::Box();
        this->resumeContainer->setAxis(brls::Axis::ROW);
        this->resumeContainer->setJustifyContent(brls::JustifyContent::FLEX_START);
        this->resumeSection->addView(this->resumeContainer);
        
        this->addView(this->resumeSection);
        this->resumeSection->setVisibility(brls::Visibility::GONE);

        // Libraries Section
        brls::Label* libTitle = new brls::Label();
        libTitle->setText("📚 My Libraries");
        libTitle->setFontSize(24);
        libTitle->setMarginBottom(15);
        this->addView(libTitle);

        this->libContainer = new brls::Box();
        this->libContainer->setAxis(brls::Axis::ROW);
        this->libContainer->setWrap(true);
        this->libContainer->setJustifyContent(brls::JustifyContent::FLEX_START);
        this->addView(this->libContainer);

        this->statusLabel = new brls::Label();
        this->statusLabel->setText("Loading...");
        this->statusLabel->setTextColor("#888888");
        this->addView(this->statusLabel);

        // Fetch data
        this->loadData();
    }

private:
    brls::Box* resumeSection;
    brls::Box* resumeContainer;
    brls::Box* libContainer;
    brls::Label* statusLabel;

    void loadData() {
        // Load Continue Watching
        EmbyClient::instance().getResumeItems([this](bool success, const std::vector<EmbyClient::EmbyItem>& items) {
            if (success && !items.empty()) {
                this->resumeSection->setVisibility(brls::Visibility::VISIBLE);
                for (const auto& item : items) {
                    this->resumeContainer->addView(new PosterCell(item));
                }
            }
        });

        // Load Libraries
        EmbyClient::instance().getUserViews([this](bool success, const std::vector<EmbyClient::EmbyItem>& items) {
            this->statusLabel->setVisibility(brls::Visibility::GONE);
            
            if (success) {
                if (items.empty()) {
                    this->statusLabel->setText("No libraries found");
                    this->statusLabel->setVisibility(brls::Visibility::VISIBLE);
                    return;
                }

                for (const auto& item : items) {
                    brls::Button* card = new brls::Button();
                    
                    // Icon based on collection type
                    std::string icon = "📁";
                    if (item.collectionType == "movies") icon = "🎬";
                    else if (item.collectionType == "tvshows") icon = "📺";
                    else if (item.collectionType == "music") icon = "🎵";
                    else if (item.collectionType == "photos") icon = "📷";
                    
                    card->setLabel(icon + " " + item.name);
                    card->setMargin(10);
                    card->setWidth(220);
                    card->setHeight(80);
                    
                    card->registerClickAction([item](brls::View* view) {
                        brls::Application::pushActivity(new LibraryActivity(item.name, item.id));
                        return true;
                    });
                    
                    this->libContainer->addView(card);
                }
            } else {
                this->statusLabel->setText("Failed to load libraries");
                this->statusLabel->setVisibility(brls::Visibility::VISIBLE);
            }
        });
    }
};

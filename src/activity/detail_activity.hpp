#pragma once
#include <borealis.hpp>
#include "api/emby_client.hpp"

class DetailActivity : public brls::Activity
{
public:
    DetailActivity(std::string itemId) : itemId(itemId) {
        brls::Box* container = new brls::Box();
        container->setAxis(brls::Axis::ROW);
        container->setPadding(40);
        container->setJustifyContent(brls::JustifyContent::FLEX_START);
        container->setAlignItems(brls::AlignItems::FLEX_START);

        // Left: Poster
        this->posterBox = new brls::Box();
        this->posterBox->setDimensions(300, 450);
        this->posterBox->setBackgroundColor(brls::RGB(30, 30, 30));
        this->posterBox->setCornerRadius(8);
        this->posterBox->setMarginRight(40);
        
        this->posterImage = new brls::Image();
        this->posterImage->setDimensions(300, 450);
        this->posterImage->setContentMode(brls::ContentMode::SCALE_ASPECT_FILL);
        this->posterImage->setCornerRadius(8);
        this->posterBox->addView(this->posterImage);
        
        container->addView(this->posterBox);

        // Right: Details
        brls::Box* detailsBox = new brls::Box();
        detailsBox->setAxis(brls::Axis::COLUMN);
        detailsBox->setGrow(1.0f);

        // Title
        this->lblTitle = new brls::Label();
        this->lblTitle->setText("Loading...");
        this->lblTitle->setFontSize(36);
        this->lblTitle->setMarginBottom(10);
        detailsBox->addView(this->lblTitle);

        // Meta (Year | Rating)
        this->lblMeta = new brls::Label();
        this->lblMeta->setText("");
        this->lblMeta->setFontSize(20);
        this->lblMeta->setTextColor("#AAAAAA");
        this->lblMeta->setMarginBottom(20);
        detailsBox->addView(this->lblMeta);

        // Buttons Row
        brls::Box* buttonRow = new brls::Box();
        buttonRow->setDirection(brls::Direction::LEFT_TO_RIGHT);
        buttonRow->setMarginBottom(30);

        this->btnPlay = new brls::Button();
        this->btnPlay->setLabel("Play");
        this->btnPlay->setStyle(brls::ButtonStyle::PRIMARY);
        this->btnPlay->registerClickAction([this](brls::View* view) {
            brls::Application::notify("Starting Playback...");
            return true;
        });
        buttonRow->addView(this->btnPlay);

        detailsBox->addView(buttonRow);

        // Overview
        this->lblOverview = new brls::Label();
        this->lblOverview->setText("");
        this->lblOverview->setFontSize(18);
        this->lblOverview->setTextColor("#CCCCCC");
        detailsBox->addView(this->lblOverview);

        container->addView(detailsBox);
        this->setContentView(container);
    }

    void onContentAvailable() override {
        // Fetch details
        EmbyClient::instance().getItem(this->itemId, [this](bool success, const EmbyClient::EmbyItem& item) {
            if (success) {
                this->lblTitle->setText(item.name);
                
                std::string meta = std::to_string(item.productionYear);
                if (item.communityRating > 0) {
                    char ratingBuf[16];
                    snprintf(ratingBuf, sizeof(ratingBuf), "%.1f", item.communityRating);
                    meta += "  |  ★ " + std::string(ratingBuf);
                }
                this->lblMeta->setText(meta);
                this->lblOverview->setText(item.overview);

                // Load Poster
                if (!item.primaryImageTag.empty()) {
                    EmbyClient::instance().downloadImage(item.id, item.primaryImageTag, [this](bool ok, const std::string& path) {
                        if (ok) this->posterImage->setImageFromFile(path);
                    });
                }

                // Load Backdrop (set as Activity background if possible, or ignore for now as Borealis Activity bg is tricky without custom draw)
                // We can set window background?
                // For now, let's skip complex backdrop rendering and focus on functionality.
                // Or maybe notify user "Backdrop found" to verify data.
            } else {
                this->lblTitle->setText("Error loading details");
            }
        });
    }

private:
    std::string itemId;
    brls::Box* posterBox;
    brls::Image* posterImage;
    brls::Label* lblTitle;
    brls::Label* lblMeta;
    brls::Label* lblOverview;
    brls::Button* btnPlay;
};

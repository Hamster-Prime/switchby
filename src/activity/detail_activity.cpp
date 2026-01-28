#include "activity/detail_activity.hpp"
#include "activity/library_activity.hpp"
#include "activity/player_activity.hpp"
#include "api/emby_client.hpp"

DetailActivity::DetailActivity(std::string itemId) : itemId(itemId) {
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
    // Action is set in onContentAvailable based on type
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

void DetailActivity::onContentAvailable() {
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

            // Logic for Type
            if (item.type == "Series" || item.type == "Season") {
                if (item.type == "Series") this->btnPlay->setLabel("View Seasons");
                else this->btnPlay->setLabel("View Episodes");
                
                this->btnPlay->registerClickAction([item](brls::View* view) {
                    // Open LibraryActivity for this parent
                    brls::Application::pushActivity(new LibraryActivity(item.name, item.id));
                    return true;
                });
            } else {
                this->btnPlay->setLabel("Play");
                this->btnPlay->registerClickAction([this, item](brls::View* view) {
                    std::string playUrl = EmbyClient::instance().getPlaybackUrl(item.id);
                    brls::Logger::info("Playing URL: {}", playUrl);
                    brls::Application::pushActivity(new PlayerActivity(playUrl));
                    return true;
                });
            }

            // Load Poster
            if (!item.primaryImageTag.empty()) {
                EmbyClient::instance().downloadImage(item.id, item.primaryImageTag, [this](bool ok, const std::string& path) {
                    if (ok) this->posterImage->setImageFromFile(path);
                });
            }
        } else {
            this->lblTitle->setText("Error loading details");
        }
    });
}

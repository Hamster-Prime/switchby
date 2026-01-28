#include "activity/detail_activity.hpp"
#include "activity/library_activity.hpp"
#include "activity/player_activity.hpp"
#include "api/emby_client.hpp"

DetailActivity::DetailActivity(std::string itemId) : itemId(itemId) {}

brls::View* DetailActivity::createContentView() {
    // Root with backdrop
    brls::Box* root = new brls::Box();
    root->setDimensions(brls::Application::contentWidth, brls::Application::contentHeight);
    
    // Backdrop image (full width, cropped)
    this->backdropImage = new brls::Image();
    this->backdropImage->setDimensions(brls::Application::contentWidth, 350);
    this->backdropImage->setContentMode(brls::ContentMode::SCALE_ASPECT_FILL);
    this->backdropImage->setVisibility(brls::Visibility::GONE);
    root->addView(this->backdropImage);

    // Gradient overlay on backdrop
    brls::Box* gradientOverlay = new brls::Box();
    gradientOverlay->setDimensions(brls::Application::contentWidth, 350);
    gradientOverlay->setPosition(0, 0);
    // Note: Real gradient would need custom draw, using semi-transparent box for now
    gradientOverlay->setBackgroundColor(brls::RGBA(20, 20, 25, 180));
    root->addView(gradientOverlay);

    // Main content container
    brls::Box* container = new brls::Box();
    container->setAxis(brls::Axis::ROW);
    container->setPadding(40);
    container->setJustifyContent(brls::JustifyContent::FLEX_START);
    container->setAlignItems(brls::AlignItems::FLEX_START);

    // Left: Poster
    this->posterBox = new brls::Box();
    this->posterBox->setDimensions(280, 420);
    this->posterBox->setBackgroundColor(brls::RGB(30, 30, 35));
    this->posterBox->setCornerRadius(12);
    this->posterBox->setMarginRight(40);
    
    this->posterImage = new brls::Image();
    this->posterImage->setDimensions(280, 420);
    this->posterImage->setContentMode(brls::ContentMode::SCALE_ASPECT_FILL);
    this->posterImage->setCornerRadius(12);
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
    this->lblMeta->setMarginBottom(25);
    detailsBox->addView(this->lblMeta);

    // Buttons Row
    brls::Box* buttonRow = new brls::Box();
    buttonRow->setAxis(brls::Axis::ROW);
    buttonRow->setMarginBottom(30);

    this->btnPlay = new brls::Button();
    this->btnPlay->setLabel("▶ Play");
    this->btnPlay->setStyle(brls::ButtonStyle::PRIMARY);
    this->btnPlay->setWidth(180);
    buttonRow->addView(this->btnPlay);

    detailsBox->addView(buttonRow);

    // Overview
    this->lblOverview = new brls::Label();
    this->lblOverview->setText("");
    this->lblOverview->setFontSize(17);
    this->lblOverview->setTextColor("#CCCCCC");
    this->lblOverview->setMaxWidth(700);
    detailsBox->addView(this->lblOverview);

    container->addView(detailsBox);
    root->addView(container);

    return root;
}

void DetailActivity::onContentAvailable() {
    EmbyClient::instance().getItem(this->itemId, [this](bool success, const EmbyClient::EmbyItem& item) {
        if (success) {
            this->lblTitle->setText(item.name);
            
            std::string meta;
            if (item.productionYear > 0) {
                meta = std::to_string(item.productionYear);
            }
            if (item.communityRating > 0) {
                char ratingBuf[16];
                snprintf(ratingBuf, sizeof(ratingBuf), "%.1f", item.communityRating);
                if (!meta.empty()) meta += "  •  ";
                meta += "★ " + std::string(ratingBuf);
            }
            if (!item.type.empty()) {
                if (!meta.empty()) meta += "  •  ";
                meta += item.type;
            }
            this->lblMeta->setText(meta);
            this->lblOverview->setText(item.overview);

            // Button logic based on type
            if (item.type == "Series") {
                this->btnPlay->setLabel("📺 Seasons");
                this->btnPlay->registerClickAction([item](brls::View* view) {
                    brls::Application::pushActivity(new LibraryActivity(item.name, item.id));
                    return true;
                });
            } else if (item.type == "Season") {
                this->btnPlay->setLabel("📺 Episodes");
                this->btnPlay->registerClickAction([item](brls::View* view) {
                    brls::Application::pushActivity(new LibraryActivity(item.name, item.id));
                    return true;
                });
            } else {
                this->btnPlay->setLabel("▶ Play");
                this->btnPlay->registerClickAction([this, item](brls::View* view) {
                    brls::Application::notify("Loading playback info...");
                    
                    EmbyClient::instance().getPlaybackInfo(item.id, [this, item](bool success, const EmbyClient::PlaybackInfo& info) {
                        if (success && !info.mediaSources.empty()) {
                            // Simple logic: Pick first source. 
                            // If DirectPlay supported, use it. Else use TranscodingUrl.
                            std::string finalUrl;
                            const auto& source = info.mediaSources[0];
                            
                            if (source.supportsDirectPlay || source.supportsDirectStream) {
                                finalUrl = source.directStreamUrl;
                                brls::Logger::info("Using Direct Play/Stream: {}", finalUrl);
                            } else if (source.supportsTranscoding && !source.transcodingUrl.empty()) {
                                finalUrl = source.transcodingUrl;
                                brls::Logger::info("Using Transcoding: {}", finalUrl);
                            } else {
                                // Fallback: Force Direct Stream even if server says no.
                                // This handles the "No Server Transcoding" + "Server thinks device is incompatible" case.
                                // MPV is powerful enough to handle most things locally.
                                if (!source.directStreamUrl.empty()) {
                                     finalUrl = source.directStreamUrl;
                                     brls::Application::notify("Forcing Direct Play (Local Decode)");
                                     brls::Logger::info("Forcing Direct Stream: {}", finalUrl);
                                } else {
                                    brls::Application::notify("Format not supported and no stream URL");
                                    return;
                                }
                            }
                            
                            brls::Application::pushActivity(new PlayerActivity(finalUrl, item.id));
                        } else {
                            brls::Application::notify("Failed to get playback info");
                        }
                    });
                    return true;
                });
            }

            // Load Poster
            if (!item.primaryImageTag.empty()) {
                EmbyClient::instance().downloadImage(item.id, item.primaryImageTag, [this](bool ok, const std::string& path) {
                    if (ok) this->posterImage->setImageFromFile(path);
                });
            }

            // Load Backdrop
            if (!item.backdropImageTag.empty()) {
                EmbyClient::instance().downloadImage(item.id, item.backdropImageTag, [this](bool ok, const std::string& path) {
                    if (ok) {
                        this->backdropImage->setImageFromFile(path);
                        this->backdropImage->setVisibility(brls::Visibility::VISIBLE);
                    }
                }, "Backdrop");
            }
        } else {
            this->lblTitle->setText("Error loading details");
        }
    });
}

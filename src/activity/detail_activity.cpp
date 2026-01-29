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
    this->backdropImage->setScalingType(brls::ImageScalingType::CROP);
    this->backdropImage->setVisibility(brls::Visibility::GONE);
    root->addView(this->backdropImage);

    // Gradient overlay on backdrop
    brls::Box* gradientOverlay = new brls::Box();
    gradientOverlay->setDimensions(brls::Application::contentWidth, 350);
    gradientOverlay->setPositionTop(0);
    gradientOverlay->setPositionLeft(0);
    gradientOverlay->setBackgroundColor(nvgRGBA(20, 20, 25, 180));
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
    this->posterBox->setBackgroundColor(nvgRGB(30, 30, 35));
    this->posterBox->setCornerRadius(12);
    this->posterBox->setMarginRight(40);
    
    this->posterImage = new brls::Image();
    this->posterImage->setDimensions(280, 420);
    this->posterImage->setScalingType(brls::ImageScalingType::CROP);
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
    this->lblMeta->setTextColor(nvgRGB(170, 170, 170));
    this->lblMeta->setMarginBottom(25);
    detailsBox->addView(this->lblMeta);

    // Buttons Row
    brls::Box* buttonRow = new brls::Box();
    buttonRow->setAxis(brls::Axis::ROW);
    buttonRow->setMarginBottom(30);

    this->btnPlay = new brls::Button();
    this->btnPlay->setText("▶ Play");
    this->btnPlay->setStyle(&brls::BUTTONSTYLE_PRIMARY);
    this->btnPlay->setWidth(180);
    buttonRow->addView(this->btnPlay);

    detailsBox->addView(buttonRow);

    // Overview
    this->lblOverview = new brls::Label();
    this->lblOverview->setText("");
    this->lblOverview->setFontSize(17);
    this->lblOverview->setTextColor(nvgRGB(204, 204, 204));
    this->lblOverview->setMaxWidth(700);
    detailsBox->addView(this->lblOverview);

    // Loading Overlay
    this->loadingOverlay = new brls::Box();
    this->loadingOverlay->setDimensions(brls::Application::contentWidth, brls::Application::contentHeight);
    this->loadingOverlay->setJustifyContent(brls::JustifyContent::CENTER);
    this->loadingOverlay->setAlignItems(brls::AlignItems::CENTER);
    this->loadingOverlay->setBackgroundColor(nvgRGBA(0, 0, 0, 180));
    this->loadingOverlay->setVisibility(brls::Visibility::GONE);
    
    brls::Label* loader = new brls::Label();
    loader->setText("⏳ Loading...");
    loader->setFontSize(32);
    this->loadingOverlay->addView(loader);
    
    // Add overlay last to cover everything
    root->addView(this->loadingOverlay);

    return root;
}

void DetailActivity::onContentAvailable() {
    EmbyClient::instance().getItem(this->itemId, [this](bool success, const EmbyClient::EmbyItem& item) {
        // Safe UI update
        brls::sync([this, success, item]() {
            if (success) {
                // Build title with episode info
                std::string displayTitle = item.name;
                if (item.type == "Episode") {
                    // Format: "S01E05 - Episode Name" or "SeriesName - S01E05"
                    char episodeInfo[32];
                    snprintf(episodeInfo, sizeof(episodeInfo), "S%02dE%02d",
                             item.parentIndexNumber, item.indexNumber);
                    if (!item.seriesName.empty()) {
                        displayTitle = item.seriesName + " - " + std::string(episodeInfo) + " - " + item.name;
                    } else {
                        displayTitle = std::string(episodeInfo) + " - " + item.name;
                    }
                } else if (item.type == "Season") {
                    // Format: "Season 1" with series name
                    if (!item.seriesName.empty()) {
                        displayTitle = item.seriesName + " - " + item.name;
                    }
                }
                this->lblTitle->setText(displayTitle);

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
                // Show runtime for movies/episodes
                if (item.runTimeTicks > 0) {
                    int runtimeMins = (int)(item.runTimeTicks / 10000000 / 60);
                    if (!meta.empty()) meta += "  •  ";
                    if (runtimeMins >= 60) {
                        meta += std::to_string(runtimeMins / 60) + "h " + std::to_string(runtimeMins % 60) + "m";
                    } else {
                        meta += std::to_string(runtimeMins) + " min";
                    }
                }
                if (!item.type.empty()) {
                    if (!meta.empty()) meta += "  •  ";
                    meta += item.type;
                }
                this->lblMeta->setText(meta);
                this->lblOverview->setText(item.overview);

                // Button logic based on type
                if (item.type == "Series") {
                    this->btnPlay->setText("📺 Seasons");
                    this->btnPlay->registerClickAction([item](brls::View* view) {
                        brls::Application::pushActivity(new LibraryActivity(item.name, item.id));
                        return true;
                    });
                } else if (item.type == "Season") {
                    this->btnPlay->setText("📺 Episodes");
                    this->btnPlay->registerClickAction([item](brls::View* view) {
                        brls::Application::pushActivity(new LibraryActivity(item.name, item.id));
                        return true;
                    });
                } else {
                    // Show resume button if there's a saved position
                    if (item.playbackPositionTicks > 0) {
                        double resumeSec = (double)item.playbackPositionTicks / 10000000.0;
                        int mins = (int)(resumeSec / 60);
                        int secs = (int)(resumeSec) % 60;
                        char resumeLabel[64];
                        snprintf(resumeLabel, sizeof(resumeLabel), "▶ Resume (%d:%02d)", mins, secs);
                        this->btnPlay->setText(resumeLabel);
                    } else {
                        this->btnPlay->setText("▶ Play");
                    }

                    this->btnPlay->registerClickAction([this, item](brls::View* view) {
                        this->loadingOverlay->setVisibility(brls::Visibility::VISIBLE);
                        brls::Application::notify("Requesting playback info...");

                        EmbyClient::instance().getPlaybackInfo(item.id, [this, item](bool success, const EmbyClient::PlaybackInfo& info) {
                            brls::sync([this, success, info, item]() {
                                this->loadingOverlay->setVisibility(brls::Visibility::GONE);

                                if (success && !info.mediaSources.empty()) {
                                    std::string finalUrl;
                                    const EmbyClient::MediaSource* bestSource = nullptr;

                                    // Selection logic (simplified for length)
                                    for (const auto& source : info.mediaSources) {
                                        if (source.supportsDirectPlay) { bestSource = &source; break; }
                                    }
                                    if (!bestSource) {
                                        for (const auto& source : info.mediaSources) {
                                            if (source.supportsDirectStream) { bestSource = &source; break; }
                                        }
                                    }
                                    if (!bestSource) {
                                        for (const auto& source : info.mediaSources) {
                                            if (source.supportsTranscoding && !source.transcodingUrl.empty()) { bestSource = &source; break; }
                                        }
                                    }
                                    if (!bestSource) bestSource = &info.mediaSources[0];

                                    const auto& source = *bestSource;

                                    if (source.supportsDirectPlay || source.supportsDirectStream) {
                                        finalUrl = source.directStreamUrl;
                                        brls::Logger::info("Using Direct Play/Stream: {}", finalUrl);
                                    } else if (source.supportsTranscoding && !source.transcodingUrl.empty()) {
                                        finalUrl = source.transcodingUrl;
                                        brls::Logger::info("Using Transcoding: {}", finalUrl);
                                    } else {
                                        if (!source.directStreamUrl.empty()) {
                                             finalUrl = source.directStreamUrl;
                                             brls::Application::notify("Forcing Direct Play (Local Decode)");
                                        } else {
                                            brls::Application::notify("Format not supported");
                                            return;
                                        }
                                    }

                                    // Pass resume position to player
                                    brls::Application::pushActivity(new PlayerActivity(finalUrl, item.id, item.playbackPositionTicks));
                                } else {
                                    brls::Application::notify("Failed to get playback info");
                                }
                            });
                        });
                        return true;
                    });
                }

                // Load Poster
                if (!item.primaryImageTag.empty()) {
                    EmbyClient::instance().downloadImage(item.id, item.primaryImageTag, [this](bool ok, const std::string& path) {
                        if (ok) {
                            brls::sync([this, path]() {
                                this->posterImage->setImageFromFile(path);
                            });
                        }
                    });
                }

                // Load Backdrop
                if (!item.backdropImageTag.empty()) {
                    EmbyClient::instance().downloadImage(item.id, item.backdropImageTag, [this](bool ok, const std::string& path) {
                        if (ok) {
                            brls::sync([this, path]() {
                                this->backdropImage->setImageFromFile(path);
                                this->backdropImage->setVisibility(brls::Visibility::VISIBLE);
                            });
                        }
                    }, "Backdrop");
                }
            } else {
                this->lblTitle->setText("Error loading details");
            }
        });
    });
}

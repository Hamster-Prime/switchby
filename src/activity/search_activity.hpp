#pragma once
#include <borealis.hpp>
#include "api/emby_client.hpp"
#include "view/poster_cell.hpp"

class SearchActivity : public brls::Activity
{
public:
    brls::View* createContentView() override
    {
        brls::Box* container = new brls::Box();
        container->setAxis(brls::Axis::COLUMN);
        container->setPadding(30);

        // Search bar
        brls::Box* searchBar = new brls::Box();
        searchBar->setAxis(brls::Axis::ROW);
        searchBar->setMarginBottom(20);
        searchBar->setAlignItems(brls::AlignItems::CENTER);

        this->btnSearch = new brls::Button();
        this->btnSearch->setLabel("🔍 Tap to search...");
        this->btnSearch->setWidth(500);
        this->btnSearch->setStyle(brls::ButtonStyle::BORDERED);
        searchBar->addView(this->btnSearch);

        container->addView(searchBar);

        // Status
        this->statusLabel = new brls::Label();
        this->statusLabel->setText("Enter a search term above");
        this->statusLabel->setTextColor("#888888");
        this->statusLabel->setMarginBottom(20);
        container->addView(this->statusLabel);

        // Results grid
        this->grid = new brls::Box();
        this->grid->setAxis(brls::Axis::ROW);
        this->grid->setWrap(true);
        this->grid->setJustifyContent(brls::JustifyContent::FLEX_START);
        container->addView(this->grid);

        return container;
    }

    void onContentAvailable() override
    {
        this->btnSearch->registerClickAction([this](brls::View* view) {
            brls::Application::getPlatform()->getImeManager()->openForText(
                [this](std::string text) {
                    if (text.empty()) return;
                    this->performSearch(text);
                },
                "Search media...", "", 100
            );
            return true;
        });
    }

private:
    brls::Button* btnSearch;
    brls::Label* statusLabel;
    brls::Box* grid;

    void performSearch(const std::string& query) {
        this->btnSearch->setLabel("🔍 " + query);
        this->statusLabel->setText("Searching...");
        this->grid->clearViews();

        EmbyClient::instance().search(query, [this](bool success, const std::vector<EmbyClient::EmbyItem>& items) {
            if (success) {
                if (items.empty()) {
                    this->statusLabel->setText("No results found");
                } else {
                    this->statusLabel->setText("Found " + std::to_string(items.size()) + " results");
                    for (const auto& item : items) {
                        this->grid->addView(new PosterCell(item));
                    }
                }
            } else {
                this->statusLabel->setText("Search failed");
            }
        });
    }
};

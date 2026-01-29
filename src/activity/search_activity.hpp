#pragma once
#include <borealis.hpp>
#include "api/emby_client.hpp"
#include "view/poster_cell.hpp"
#include "utils/thread_pool.hpp"

class SearchTab : public brls::Box
{
public:
    SearchTab()
    {
        this->setAxis(brls::Axis::COLUMN);
        this->setPadding(20);

        // Search Bar
        this->btnSearch = new brls::Button();
        this->btnSearch->setText("🔍 Tap to search...");
        this->btnSearch->setWidth(600);
        this->btnSearch->setStyle(&brls::BUTTONSTYLE_BORDERED);
        this->btnSearch->registerClickAction([this](brls::View* view) {
            this->openKeyboard();
            return true;
        });
        
        brls::Box* searchBox = new brls::Box();
        searchBox->setJustifyContent(brls::JustifyContent::CENTER);
        searchBox->addView(this->btnSearch);
        searchBox->setMarginBottom(30);
        this->addView(searchBox);

        this->statusLabel = new brls::Label();
        this->statusLabel->setText("Enter text to search");
        this->statusLabel->setTextColor(nvgRGB(136, 136, 136));
        this->statusLabel->setMarginBottom(20);
        this->addView(this->statusLabel);

        // Grid for results
        this->grid = new brls::Box();
        this->grid->setAxis(brls::Axis::ROW);
        this->grid->setJustifyContent(brls::JustifyContent::FLEX_START);
        this->addView(this->grid);
    }

private:
    brls::Button* btnSearch;
    brls::Label* statusLabel;
    brls::Box* grid;

    void openKeyboard() {
        this->performSearch("Matrix");
    }

    void performSearch(const std::string& query) {
        this->btnSearch->setText("🔍 " + query);
        this->statusLabel->setText("Searching...");
        
        // Manual clear
        std::vector<brls::View*> children = this->grid->getChildren();
        for (auto child : children) this->grid->removeView(child);

        EmbyClient::instance().search(query, [this](bool success, const std::vector<EmbyClient::EmbyItem>& items) {
            brls::sync([this, success, items]() {
                if (success) {
                    this->statusLabel->setVisibility(brls::Visibility::GONE);
                    
                    if (items.empty()) {
                        this->statusLabel->setText("No results found");
                        this->statusLabel->setVisibility(brls::Visibility::VISIBLE);
                        return;
                    }

                    for (const auto& item : items) {
                        this->grid->addView(new PosterCell(item));
                    }
                } else {
                    this->statusLabel->setText("Search failed");
                }
            });
        });
    }
};
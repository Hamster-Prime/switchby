#pragma once
#include <borealis.hpp>
#include "api/emby_client.hpp"
#include "view/poster_cell.hpp"

class LibraryActivity : public brls::Activity
{
public:
    LibraryActivity(std::string libraryName, std::string libraryId) 
        : libraryName(libraryName), libraryId(libraryId) {}

    brls::View* createContentView() override
    {
        brls::Box* container = new brls::Box();
        container->setAxis(brls::Axis::COLUMN);
        container->setPadding(20);

        // Header
        brls::Box* header = new brls::Box();
        header->setAxis(brls::Axis::ROW);
        header->setAlignItems(brls::AlignItems::CENTER);
        header->setMarginBottom(20);

        brls::Label* title = new brls::Label();
        title->setText(this->libraryName);
        title->setFontSize(28);
        header->addView(title);

        container->addView(header);

        this->statusLabel = new brls::Label();
        this->statusLabel->setText("Loading items...");
        this->statusLabel->setTextColor("#888888");
        container->addView(this->statusLabel);

        // Grid for items
        this->grid = new brls::Box();
        this->grid->setAxis(brls::Axis::ROW);
        this->grid->setWrap(true);
        this->grid->setJustifyContent(brls::JustifyContent::FLEX_START);
        container->addView(this->grid);

        return container;
    }

    void onContentAvailable() override
    {
        EmbyClient::instance().getItems(this->libraryId, [this](bool success, const std::vector<EmbyClient::EmbyItem>& items) {
            if (success) {
                this->statusLabel->setVisibility(brls::Visibility::GONE);
                
                if (items.empty()) {
                    this->statusLabel->setText("No items found");
                    this->statusLabel->setVisibility(brls::Visibility::VISIBLE);
                    return;
                }

                for (const auto& item : items) {
                    this->grid->addView(new PosterCell(item));
                }
            } else {
                this->statusLabel->setText("Failed to load items");
            }
        });
    }

private:
    std::string libraryName;
    std::string libraryId;
    brls::Label* statusLabel;
    brls::Box* grid;
};

#pragma once
#include <borealis.hpp>
#include "api/emby_client.hpp"
#include "view/poster_cell.hpp"

class LibraryActivity : public brls::Activity
{
public:
    LibraryActivity(std::string libraryName, std::string libraryId) 
        : libraryName(libraryName), libraryId(libraryId)
    {
        brls::Box* container = new brls::Box();
        container->setAxis(brls::Axis::COLUMN);
        container->setPadding(20);

        brls::Label* title = new brls::Label();
        title->setText(this->libraryName);
        title->setFontSize(28);
        title->setMarginBottom(20);
        container->addView(title);

        this->statusLabel = new brls::Label();
        this->statusLabel->setText("Loading items...");
        this->statusLabel->setTextColor("#888888");
        container->addView(this->statusLabel);

        // Grid for items
        this->grid = new brls::Box();
        this->grid->setDirection(brls::Direction::LEFT_TO_RIGHT);
        this->grid->setWrap(true);
        this->grid->setJustifyContent(brls::JustifyContent::FLEX_START);
        
        // Wrap grid in a scrolling frame
        brls::AppletFrame* frame = new brls::AppletFrame(container);
        frame->setContentView(this->grid);
        
        // Note: AppletFrame usually manages the header/content structure. 
        // For simplicity in this raw impl, we add grid to container, and container to view.
        // But for scrolling, let's use a wrapper logic or assume Borealis handles scrolling if content overflows.
        // Actually, AppletFrame is a high level component. Let's just use the Activity's default content view which handles scrolling if configured or use a ScrollView.
        // Borealis's Activity::setContentView usually takes a view. 
        
        // Let's use a ScrollView for safety if list is long
        // (Assuming standard Borealis components available, if not, standard Box usually scrolls in some contexts or needs a parent)
        // We'll stick to a simple Box for now, Borealis automatic navigation usually handles it.
        
        container->addView(this->grid);
        this->setContentView(container);
    }

    void onContentAvailable() override
    {
        EmbyClient::instance().getItems(this->libraryId, [this](bool success, const std::vector<EmbyClient::EmbyItem>& items) {
            if (success) {
                this->statusLabel->setVisibility(brls::Visibility::GONE);
                
                if (items.empty()) {
                    this->statusLabel->setText("No items found.");
                    this->statusLabel->setVisibility(brls::Visibility::VISIBLE);
                    return;
                }

                for (const auto& item : items) {
                    this->grid->addView(new PosterCell(item));
                }
            } else {
                this->statusLabel->setText("Failed to load items.");
            }
        });
    }

private:
    std::string libraryName;
    std::string libraryId;
    brls::Label* statusLabel;
    brls::Box* grid;
};

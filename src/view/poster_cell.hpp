#pragma once
#include <borealis.hpp>
#include "api/emby_client.hpp"

class PosterCell : public brls::Box
{
public:
    PosterCell(const EmbyClient::EmbyItem& item)
    {
        this->setDimensions(160, 280);
        this->setDirection(brls::Direction::TOP_TO_BOTTOM);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setMargin(10);
        
        // Image Container
        this->imageBox = new brls::Box();
        this->imageBox->setDimensions(160, 240);
        this->imageBox->setBackgroundColor(brls::RGB(50, 50, 50)); // Placeholder color
        this->imageBox->setCornerRadius(6);
        this->addView(this->imageBox);
        
        // Label
        this->label = new brls::Label();
        this->label->setText(item.name);
        this->label->setFontSize(18);
        this->label->setMarginTop(8);
        this->label->setMaxWidth(160);
        this->label->setSingleLine(true); // Truncate with ellipsis if possible or just clip
        this->addView(this->label);

        // Make clickable
        this->registerClickAction([item](brls::View* view) {
            brls::Application::notify("Playing: " + item.name);
            return true;
        });

        // Add focus effect
        this->getFocusEvent()->subscribe([this](bool focused) {
            if (focused) {
                this->imageBox->setBorderThickness(4);
                this->imageBox->setBorderColor(brls::Application::getTheme().colorPrimary);
                this->setScale(1.05f);
            } else {
                this->imageBox->setBorderThickness(0);
                this->setScale(1.0f);
            }
        });

        // Load Image
        if (!item.primaryImageTag.empty()) {
            EmbyClient::instance().downloadImage(item.id, item.primaryImageTag, [this](bool success, const std::string& path) {
                if (success && !path.empty()) {
                    // Create image view and replace background
                    brls::Image* img = new brls::Image();
                    img->setImageFromFile(path);
                    img->setDimensions(160, 240);
                    img->setContentMode(brls::ContentMode::SCALE_ASPECT_FILL);
                    img->setCornerRadius(6);
                    
                    this->imageBox->clearViews();
                    this->imageBox->addView(img);
                }
            });
        }
    }

private:
    brls::Box* imageBox;
    brls::Label* label;
};

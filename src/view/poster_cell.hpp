#pragma once
#include <borealis.hpp>
#include "api/emby_client.hpp"
#include "activity/detail_activity.hpp"

class PosterCell : public brls::Box
{
public:
    PosterCell(const EmbyClient::EmbyItem& item)
    {
        this->setDimensions(160, 280);
        this->setAxis(brls::Axis::COLUMN);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setMargins(8, 8, 8, 8);
        this->setFocusable(true);

        // Image Container with shadow effect
        this->imageBox = new brls::Box();
        this->imageBox->setDimensions(160, 240);
        this->imageBox->setBackgroundColor(nvgRGB(40, 40, 45));
        this->imageBox->setCornerRadius(8);
        this->addView(this->imageBox);
        
        // Label
        this->label = new brls::Label();
        this->label->setText(item.name);
        this->label->setFontSize(16);
        this->label->setMarginTop(8);
        this->label->setMaxWidth(155);
        this->label->setSingleLine(true);
        this->addView(this->label);

        // Type badge (small label showing Movie/Series/Episode)
        if (!item.type.empty() && item.type != "Movie") {
            brls::Label* badge = new brls::Label();
            badge->setText(item.type);
            badge->setFontSize(12);
            badge->setTextColor(nvgRGB(136, 136, 136));
            this->addView(badge);
        }

        // Make clickable
        this->registerClickAction([item](brls::View* view) {
            brls::Application::pushActivity(new DetailActivity(item.id));
            return true;
        });

        // Focus animation
        this->getFocusEvent()->subscribe([this](bool focused) {
            if (focused) {
                // Border thickness removed, simpler focus
                this->label->setTextColor(nvgRGB(255, 255, 255));
            } else {
                this->label->setTextColor(nvgRGB(204, 204, 204));
            }
        });

        // Load Image
        if (!item.primaryImageTag.empty()) {
            EmbyClient::instance().downloadImage(item.id, item.primaryImageTag, [this](bool success, const std::string& path) {
                if (success && !path.empty()) {
                    brls::Image* img = new brls::Image();
                    img->setImageFromFile(path);
                    img->setDimensions(160, 240);
                    // Content mode removed/simplified as it might be missing
                    img->setScalingType(brls::ImageScalingType::FIT); 
                    img->setCornerRadius(8);
                    
                    this->imageBox->clearViews();
                    this->imageBox->addView(img);
                }
            });
        } else {
            // Placeholder icon
            brls::Label* placeholder = new brls::Label();
            placeholder->setText("🎬");
            placeholder->setFontSize(48);
            placeholder->setHorizontalAlign(brls::HorizontalAlign::CENTER);
            placeholder->setVerticalAlign(brls::VerticalAlign::CENTER);
            this->imageBox->setJustifyContent(brls::JustifyContent::CENTER);
            this->imageBox->setAlignItems(brls::AlignItems::CENTER);
            this->imageBox->addView(placeholder);
        }
    }

private:
    brls::Box* imageBox;
    brls::Label* label;
};

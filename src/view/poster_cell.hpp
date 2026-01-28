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
        this->setMargin(8);
        this->setFocusable(true);

        // Image Container with shadow effect
        this->imageBox = new brls::Box();
        this->imageBox->setDimensions(160, 240);
        this->imageBox->setBackgroundColor(brls::RGB(40, 40, 45));
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
            badge->setTextColor("#888888");
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
                this->imageBox->setBorderThickness(3);
                this->imageBox->setBorderColor(brls::Application::getTheme().colorPrimary);
                this->setScale(1.08f);
                this->label->setTextColor("#FFFFFF");
            } else {
                this->imageBox->setBorderThickness(0);
                this->setScale(1.0f);
                this->label->setTextColor("#CCCCCC");
            }
        });

        // Load Image
        if (!item.primaryImageTag.empty()) {
            EmbyClient::instance().downloadImage(item.id, item.primaryImageTag, [this](bool success, const std::string& path) {
                if (success && !path.empty()) {
                    brls::Image* img = new brls::Image();
                    img->setImageFromFile(path);
                    img->setDimensions(160, 240);
                    img->setContentMode(brls::ContentMode::SCALE_ASPECT_FILL);
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

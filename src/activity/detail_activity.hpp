#pragma once
#include <borealis.hpp>

class DetailActivity : public brls::Activity
{
public:
    DetailActivity(std::string itemId);
    void onContentAvailable() override;

private:
    std::string itemId;
    brls::Box* posterBox;
    brls::Image* posterImage;
    brls::Label* lblTitle;
    brls::Label* lblMeta;
    brls::Label* lblOverview;
    brls::Button* btnPlay;
};

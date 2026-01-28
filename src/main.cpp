#include <borealis.hpp>
#include "activity/server_select_activity.hpp"

using namespace brls::literals;

int main(int argc, char* argv[])
{
    // Initialize the logger
    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    // Init the app
    if (!brls::Application::init("SwitchBy - Emby Client"))
    {
        brls::Logger::error("Unable to init Borealis application");
        return EXIT_FAILURE;
    }

    // Creating views
    brls::Application::pushActivity(new ServerSelectActivity());

    // Run the app
    while (brls::Application::mainLoop());

    // Exit
    return EXIT_SUCCESS;
}

#include <borealis.hpp>
#include "activity/server_select_activity.hpp"

// No longer need manual main thread dispatching,
// assuming the updated borealis library handles this.
// Use brls::sync(...) directly where needed in other parts of the code.

int main(int argc, char* argv[])
{
    // Initialize the logger
    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    // Init the app
    if (!brls::Application::init())
    {
        brls::Logger::error("Unable to init Borealis application");
        return EXIT_FAILURE;
    }

    // Creating views
    brls::Application::pushActivity(new ServerSelectActivity());

    // Run the app
    while (brls::Application::mainLoop())
    {
        // The main loop is now much simpler.
        // The borealis backend will handle events and rendering.
    }

    // Exit
    return EXIT_SUCCESS;
}

#include <borealis.hpp>
#include "activity/server_select_activity.hpp"

#ifdef __SWITCH__
#include <switch.h>
#endif

int main(int argc, char* argv[])
{
    // Initialize the logger
    brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);

#ifdef __SWITCH__
    // Initialize Socket (Network)
    // Allocate a large buffer for socket service to prevent memory exhaustion
    socketInitializeDefault();
    
    // Initialize nxlink if available (for debugging via USB/Network)
    nxlinkStdio();
#endif

    try {
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
            // The main loop handles events and rendering
        }
    } catch (const std::exception& e) {
        brls::Logger::error("Main loop exception: {}", e.what());
        // On Switch, maybe showing a native error dialog would be better, 
        // but logger is safer for now.
    } catch (...) {
        brls::Logger::error("Unknown exception in main loop");
    }

#ifdef __SWITCH__
    socketExit();
#endif

    // Exit
    return EXIT_SUCCESS;
}

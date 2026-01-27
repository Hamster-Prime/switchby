#include <borealis.hpp>
#include <string>

using namespace brls::literals;

class MainActivity : public brls::Activity
{
public:
    CONTENT_FROM_XML_RES("activity/main.xml");
};

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
    brls::Application::pushActivity(new MainActivity());

    // Run the app
    while (brls::Application::mainLoop());

    // Exit
    return EXIT_SUCCESS;
}

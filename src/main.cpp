#include <borealis.hpp>
#include "activity/server_select_activity.hpp"
#include <queue>
#include <mutex>
#include <functional>

// Global Main Thread Dispatcher
// Since the older borealis lib lacks brls::sync, we implement it manually.
static std::queue<std::function<void()>> mainThreadTasks;
static std::mutex mainThreadMutex;

namespace brls {
    void sync(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mainThreadMutex);
        mainThreadTasks.push(task);
    }
}

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
    while (brls::Application::mainLoop()) {
        // Process main thread tasks
        std::function<void()> task;
        {
            std::lock_guard<std::mutex> lock(mainThreadMutex);
            if (!mainThreadTasks.empty()) {
                task = std::move(mainThreadTasks.front());
                mainThreadTasks.pop();
            }
        }
        
        if (task) {
            try {
                task();
            } catch(...) {
                brls::Logger::error("Main thread task failed");
            }
        }
    }

    // Exit
    return EXIT_SUCCESS;
}

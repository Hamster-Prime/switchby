#pragma once
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <borealis.hpp>

// A simple thread pool to prevent thread explosion on Switch
class ThreadPool {
public:
    static ThreadPool& instance() {
        static ThreadPool instance(4); // limit to 4 concurrent network tasks
        return instance;
    }

    void submit(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.push(task);
        }
        condition.notify_one();
    }

    // Stop all threads (for app exit)
    void stop() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            shouldStop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }

private:
    ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this] {
                            return this->shouldStop || !this->tasks.empty();
                        });
                        
                        if (this->shouldStop && this->tasks.empty()) return;
                        
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    
                    try {
                        task();
                    } catch (const std::exception& e) {
                        brls::Logger::error("ThreadPool task failed: {}", e.what());
                    }
                }
            });
        }
    }

    ~ThreadPool() { stop(); }

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool shouldStop = false;
};

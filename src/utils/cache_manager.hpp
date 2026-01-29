#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <utime.h>
#include <borealis.hpp>

// Simple LRU cache manager for image files
// Limits cache size and removes oldest files when limit exceeded
class CacheManager {
public:
    static CacheManager& instance() {
        static CacheManager instance;
        return instance;
    }

    // Set max cache size in bytes (default 100MB)
    void setMaxCacheSize(size_t bytes) {
        maxCacheSize = bytes;
    }

    // Get cache directory path
    std::string getCacheDir() {
        #ifdef __SWITCH__
        return "/switch/switchby/cache";
        #else
        return "cache";
        #endif
    }

    // Ensure cache directory exists
    void ensureCacheDir() {
        std::string dir = getCacheDir();
        mkdir(dir.c_str(), 0777);
    }

    // Update access time of a file (mark as recently used)
    void touchFile(const std::string& path) {
        // Update file's access time to now
        utime(path.c_str(), nullptr);
    }

    // Check cache size and clean if needed
    void checkAndClean() {
        std::string dir = getCacheDir();

        // Collect all cache files with their info
        std::vector<CacheFile> files;
        size_t totalSize = 0;

        DIR* d = opendir(dir.c_str());
        if (!d) return;

        struct dirent* entry;
        while ((entry = readdir(d)) != nullptr) {
            if (entry->d_type == DT_REG) {
                std::string path = dir + "/" + entry->d_name;
                struct stat st;
                if (stat(path.c_str(), &st) == 0) {
                    files.push_back({path, (size_t)st.st_size, st.st_atime});
                    totalSize += st.st_size;
                }
            }
        }
        closedir(d);

        // If under limit, nothing to do
        if (totalSize <= maxCacheSize) {
            return;
        }

        // Sort by access time (oldest first)
        std::sort(files.begin(), files.end(), [](const CacheFile& a, const CacheFile& b) {
            return a.accessTime < b.accessTime;
        });

        // Remove oldest files until under limit
        size_t targetSize = maxCacheSize * 0.8; // Clean to 80% of max
        size_t removed = 0;

        for (const auto& file : files) {
            if (totalSize <= targetSize) break;

            if (remove(file.path.c_str()) == 0) {
                totalSize -= file.size;
                removed++;
            }
        }

        if (removed > 0) {
            brls::Logger::info("Cache cleanup: removed {} old files", removed);
        }
    }

    // Get current cache stats
    void getStats(size_t& outTotalSize, int& outFileCount) {
        std::string dir = getCacheDir();
        outTotalSize = 0;
        outFileCount = 0;

        DIR* d = opendir(dir.c_str());
        if (!d) return;

        struct dirent* entry;
        while ((entry = readdir(d)) != nullptr) {
            if (entry->d_type == DT_REG) {
                std::string path = dir + "/" + entry->d_name;
                struct stat st;
                if (stat(path.c_str(), &st) == 0) {
                    outTotalSize += st.st_size;
                    outFileCount++;
                }
            }
        }
        closedir(d);
    }

private:
    CacheManager() : maxCacheSize(100 * 1024 * 1024) {} // 100MB default

    size_t maxCacheSize;

    struct CacheFile {
        std::string path;
        size_t size;
        time_t accessTime;
    };
};

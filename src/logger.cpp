#include "logger.h"

#include <chrono>
#include <cstdarg>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

std::deque<std::string> recentLogs;
std::mutex logMutex;
std::ofstream logFile;
constexpr std::size_t kMaxRecentLogs = 1000;

}  // namespace

namespace logging {

void Initialize() {
    try {
        // Create logs directory
#ifdef _WIN32
        CreateDirectoryA("logs", nullptr);
#endif

        // Generate timestamped filename
        const auto now = std::chrono::system_clock::now();
        const auto timeT = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &timeT);
#else
        localtime_r(&timeT, &tm);
#endif

        std::ostringstream filename;
        filename << "logs/app_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".log";

        logFile.open(filename.str());

        LogMessage("INFO", "Logging initialized");

    } catch (...) {
#ifdef _WIN32
        OutputDebugStringA("Log init failed");
#endif
    }
}

void Shutdown() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

std::string format(const char* fmt, ...) {
    if (!fmt)
        return {};
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);
    if (len <= 0) {
        va_end(args);
        return {};
    }
    std::string buf;
    buf.resize(static_cast<size_t>(len));
    vsnprintf(buf.data(), static_cast<size_t>(len) + 1, fmt, args);
    va_end(args);
    return buf;
}

void LogMessage(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);

    const auto now = std::chrono::system_clock::now();
    const auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif

    std::ostringstream oss;
    oss << "[" << std::put_time(&tm, "%H:%M:%S") << "] [" << level << "] " << message;

    const std::string logEntry = oss.str();

    // Console output
    std::cout << logEntry << std::endl;

    // File output
    if (logFile.is_open()) {
        logFile << logEntry << std::endl;
        logFile.flush();
    }

    // Store for ImGui
    recentLogs.push_back(logEntry);
    if (recentLogs.size() > kMaxRecentLogs) {
        recentLogs.pop_front();
    }

#ifdef _WIN32
    OutputDebugStringA((logEntry + "\n").c_str());
#endif
}

std::vector<std::string> GetRecentLogs(std::size_t maxLines) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::vector<std::string> result;
    const std::size_t start = recentLogs.size() > maxLines ? recentLogs.size() - maxLines : 0;
    for (std::size_t i = start; i < recentLogs.size(); ++i) {
        result.push_back(recentLogs[i]);
    }
    return result;
}

}  // namespace logging
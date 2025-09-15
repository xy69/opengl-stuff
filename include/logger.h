#pragma once

#include <string>
#include <vector>

namespace logging {

void Initialize();
void Shutdown();
std::vector<std::string> GetRecentLogs(std::size_t maxLines = 100);
void LogMessage(const std::string& level, const std::string& message);

// printf-style formatting helper
std::string format(const char* fmt, ...);

}  // namespace logging

// Variadic logging macros supporting printf-style formatting
#define LOG_TRACE(...) logging::LogMessage("TRACE", logging::format(__VA_ARGS__))
#define LOG_DEBUG(...) logging::LogMessage("DEBUG", logging::format(__VA_ARGS__))
#define LOG_INFO(...) logging::LogMessage("INFO", logging::format(__VA_ARGS__))
#define LOG_WARN(...) logging::LogMessage("WARN", logging::format(__VA_ARGS__))
#define LOG_ERROR(...) logging::LogMessage("ERROR", logging::format(__VA_ARGS__))

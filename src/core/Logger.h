#pragma once
// ═══════════════════════════════════════════════════════════════════
//  Nova2D Logger — Color-coded console logging
// ═══════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstdio>
#include <string>
#include <ctime>
#include <sstream>

namespace Nova {

enum class LogLevel {
    Trace,
    Info,
    Warn,
    Error,
    Fatal
};

class Logger {
public:
    static LogLevel minLevel;

    static void Log(LogLevel level, const char* file, int line, const std::string& msg) {
        if (level < minLevel) return;

        const char* levelStr = "";
        const char* colorCode = "";
        const char* resetCode = "\033[0m";

        switch (level) {
            case LogLevel::Trace: levelStr = "TRACE"; colorCode = "\033[90m"; break;   // Gray
            case LogLevel::Info:  levelStr = "INFO "; colorCode = "\033[36m"; break;   // Cyan
            case LogLevel::Warn:  levelStr = "WARN "; colorCode = "\033[33m"; break;   // Yellow
            case LogLevel::Error: levelStr = "ERROR"; colorCode = "\033[31m"; break;   // Red
            case LogLevel::Fatal: levelStr = "FATAL"; colorCode = "\033[1;31m"; break; // Bold Red
        }

        // Get timestamp
        auto now = std::time(nullptr);
        auto* tm = std::localtime(&now);
        char timeBuf[16];
        std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", tm);

        // Extract just filename from path
        std::string filePath(file);
        size_t lastSlash = filePath.find_last_of("/\\");
        std::string fileName = (lastSlash != std::string::npos) 
            ? filePath.substr(lastSlash + 1) : filePath;

        // Print formatted log
        std::printf("%s[%s] [%s] %s:%d → %s%s\n", 
            colorCode, timeBuf, levelStr, 
            fileName.c_str(), line, msg.c_str(), resetCode);

        if (level == LogLevel::Fatal) {
            std::abort();
        }
    }

    // Simple format helper using ostringstream
    template<typename... Args>
    static std::string Format(const Args&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        return oss.str();
    }
};

inline LogLevel Logger::minLevel = LogLevel::Trace;

} // namespace Nova

// ─── Convenience Macros ─────────────────────────────────────────
#define NOVA_TRACE(...)  Nova::Logger::Log(Nova::LogLevel::Trace, __FILE__, __LINE__, Nova::Logger::Format(__VA_ARGS__))
#define NOVA_LOG(...)    Nova::Logger::Log(Nova::LogLevel::Info,  __FILE__, __LINE__, Nova::Logger::Format(__VA_ARGS__))
#define NOVA_WARN(...)   Nova::Logger::Log(Nova::LogLevel::Warn,  __FILE__, __LINE__, Nova::Logger::Format(__VA_ARGS__))
#define NOVA_ERROR(...)  Nova::Logger::Log(Nova::LogLevel::Error, __FILE__, __LINE__, Nova::Logger::Format(__VA_ARGS__))
#define NOVA_FATAL(...)  Nova::Logger::Log(Nova::LogLevel::Fatal, __FILE__, __LINE__, Nova::Logger::Format(__VA_ARGS__))

// Assert
#define NOVA_ASSERT(condition, ...) \
    do { if (!(condition)) { NOVA_FATAL("Assertion failed: " #condition " — ", __VA_ARGS__); } } while(0)

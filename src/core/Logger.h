#pragma once

#include <cstdio>
#include <cstdarg>
#include <string>
#include <chrono>
#include <mutex>

namespace ps {

enum class LogLevel {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel level) { m_minLevel = level; }
    LogLevel getLevel() const { return m_minLevel; }

    void trace(const char* fmt, ...);
    void debug(const char* fmt, ...);
    void info(const char* fmt, ...);
    void warn(const char* fmt, ...);
    void error(const char* fmt, ...);
    void fatal(const char* fmt, ...);

    // File logging (optional, set before first log call)
    void setLogFile(const std::string& path);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const char* fmt, va_list args);
    const char* levelStr(LogLevel level) const;
    const char* levelColor(LogLevel level) const;

    LogLevel m_minLevel = LogLevel::INFO;
    FILE* m_file = nullptr;
    std::mutex m_mutex;
    std::chrono::steady_clock::time_point m_startTime = std::chrono::steady_clock::now();
};

// Convenience macros
#define LOG_TRACE(...) ps::Logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) ps::Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...)  ps::Logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...)  ps::Logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) ps::Logger::instance().error(__VA_ARGS__)
#define LOG_FATAL(...) ps::Logger::instance().fatal(__VA_ARGS__)

} // namespace ps

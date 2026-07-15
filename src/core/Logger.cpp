#include "core/Logger.h"

namespace ps {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
}

void Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file) fclose(m_file);
    m_file = fopen(path.c_str(), "w");
    if (!m_file) {
        fprintf(stderr, "[Logger] Failed to open log file: %s\n", path.c_str());
    }
}

void Logger::trace(const char* fmt, ...) {
    va_list args; va_start(args, fmt); log(LogLevel::TRACE, fmt, args); va_end(args);
}
void Logger::debug(const char* fmt, ...) {
    va_list args; va_start(args, fmt); log(LogLevel::DEBUG, fmt, args); va_end(args);
}
void Logger::info(const char* fmt, ...) {
    va_list args; va_start(args, fmt); log(LogLevel::INFO, fmt, args); va_end(args);
}
void Logger::warn(const char* fmt, ...) {
    va_list args; va_start(args, fmt); log(LogLevel::WARN, fmt, args); va_end(args);
}
void Logger::error(const char* fmt, ...) {
    va_list args; va_start(args, fmt); log(LogLevel::ERROR, fmt, args); va_end(args);
}
void Logger::fatal(const char* fmt, ...) {
    va_list args; va_start(args, fmt); log(LogLevel::FATAL, fmt, args); va_end(args);
}

void Logger::log(LogLevel level, const char* fmt, va_list args) {
    if (level < m_minLevel) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto elapsed = std::chrono::steady_clock::now() - m_startTime;
    double seconds = std::chrono::duration<double>(elapsed).count();

    // ANSI color codes for terminal
    const char* color = levelColor(level);
    const char* reset = "\033[0m";
    const char* lvl = levelStr(level);

    // Print to stdout/stderr
    FILE* out = (level >= LogLevel::ERROR) ? stderr : stdout;
    fprintf(out, "%s[%8.3fs] [%-5s] ", color, seconds, lvl);
    vfprintf(out, fmt, args);
    fprintf(out, "%s\n", reset);

    // Print to file (no color codes)
    if (m_file) {
        fprintf(m_file, "[%8.3fs] [%-5s] ", seconds, lvl);
        va_list args2;
        va_copy(args2, args);
        vfprintf(m_file, fmt, args2);
        va_end(args2);
        fprintf(m_file, "\n");
        fflush(m_file);
    }
}

const char* Logger::levelStr(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
    }
    return "???";
}

const char* Logger::levelColor(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE: return "\033[90m";      // Dark gray
        case LogLevel::DEBUG: return "\033[36m";      // Cyan
        case LogLevel::INFO:  return "\033[32m";      // Green
        case LogLevel::WARN:  return "\033[33m";      // Yellow
        case LogLevel::ERROR: return "\033[31m";      // Red
        case LogLevel::FATAL: return "\033[35;1m";    // Bold magenta
    }
    return "\033[0m";
}

} // namespace ps

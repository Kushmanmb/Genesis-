#pragma once

#include <iostream>
#include <string>

// Lightweight debug logger.
//
// Logging is controlled at compile time by the GENESIS_DEBUG preprocessor
// symbol.  When GENESIS_DEBUG is defined each LOG_* call emits a labelled
// line to std::cerr; when it is not defined all calls compile away to
// nothing, so there is zero runtime overhead in release builds.
//
// Typical usage:
//   LOG_DEBUG("Node started");
//   LOG_INFO("Block added: " + data);
//   LOG_WARNING("Unexpected state");
//   LOG_ERROR("Permission denied for caller: " + addr);

class Logger {
public:
    enum class Level { DEBUG, INFO, WARNING, ERROR };

    // Emit a labelled log message to std::cerr.
    static void log(Level level, const std::string &message) {
        std::cerr << "[" << levelToString(level) << "] " << message << "\n";
    }

private:
    static const char *levelToString(Level level) {
        switch (level) {
            case Level::DEBUG:   return "DEBUG";
            case Level::INFO:    return "INFO";
            case Level::WARNING: return "WARNING";
            case Level::ERROR:   return "ERROR";
        }
        return "UNKNOWN";
    }
};

#ifdef GENESIS_DEBUG
#   define LOG_DEBUG(msg)   Logger::log(Logger::Level::DEBUG,   (msg))
#   define LOG_INFO(msg)    Logger::log(Logger::Level::INFO,    (msg))
#   define LOG_WARNING(msg) Logger::log(Logger::Level::WARNING, (msg))
#   define LOG_ERROR(msg)   Logger::log(Logger::Level::ERROR,   (msg))
#else
// In non-debug builds the argument is wrapped in a dead branch so the
// compiler eliminates it entirely while still type-checking the expression.
#   define LOG_DEBUG(msg)   do { if (false) { Logger::log(Logger::Level::DEBUG,   (msg)); } } while (false)
#   define LOG_INFO(msg)    do { if (false) { Logger::log(Logger::Level::INFO,    (msg)); } } while (false)
#   define LOG_WARNING(msg) do { if (false) { Logger::log(Logger::Level::WARNING, (msg)); } } while (false)
#   define LOG_ERROR(msg)   do { if (false) { Logger::log(Logger::Level::ERROR,   (msg)); } } while (false)
#endif

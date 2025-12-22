#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <sstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>
#include <sys/time.h>

#include "Log.pb.h"
#include "std.pb.h"

namespace core {

// Forward declarations
class Logger;
class LogStream;

/**
 * @brief Log severity levels (compatible with ROS logging system)
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

/**
 * @brief Convert LogLevel to string representation
 */
inline const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "UNKNOWN";
    }
}

/**
 * @brief Convert LogLevel to protobuf enum
 */
inline log_msg::LogLevel toProtoLevel(LogLevel level) {
    return static_cast<log_msg::LogLevel>(static_cast<int>(level));
}

/**
 * @brief Callback type for remote publishing
 * User provides this callback to send LogEntry via their own Publisher
 */
using LogPublishCallback = std::function<void(const log_msg::LogEntry&)>;

/**
 * @brief Log stream class for stream-style logging
 * @example LOG_INFO(logger) << "value: " << 123;
 */
class LogStream {
public:
    LogStream(Logger& logger, LogLevel level);
    ~LogStream();
    
    // Disable copy
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;
    
    // Enable move
    LogStream(LogStream&& other) noexcept;
    
    // Stream operator
    template<typename T>
    LogStream& operator<<(const T& value) {
        if (active_) {
            ss_ << value;
        }
        return *this;
    }
    
private:
    Logger& logger_;
    LogLevel level_;
    std::ostringstream ss_;
    bool active_;
};

/**
 * @brief Logger class for logging messages
 * 
 * Features:
 * - Supports DEBUG/INFO/WARN/ERROR/FATAL levels (ROS compatible)
 * - Stream-style output: LOG_INFO(logger) << "message";
 * - Real-time output to console and remote (via callback)
 * - Thread-safe
 * - Configurable minimum log level filtering
 */
class Logger {
public:
    /**
     * @brief Constructor
     * @param node_name Node name identifier
     * @param publish_callback Callback for remote publishing (optional)
     */
    Logger(const std::string& node_name, 
           LogPublishCallback publish_callback = nullptr);
    
    ~Logger() = default;
    
    // Disable copy
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    /**
     * @brief Set publish callback for remote logging
     */
    void setPublishCallback(LogPublishCallback callback);
    
    /**
     * @brief Set minimum log level for filtering
     */
    void setMinLevel(LogLevel level);
    
    /**
     * @brief Get current minimum log level
     */
    LogLevel getMinLevel() const;
    
    /**
     * @brief Enable/disable local console output
     */
    void setLocalOutput(bool enabled);
    
    /**
     * @brief Enable/disable remote publishing
     */
    void setRemoteOutput(bool enabled);
    
    /**
     * @brief Log a message at specified level
     */
    void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Create a log stream for stream-style logging
     */
    LogStream stream(LogLevel level);
    
    /**
     * @brief Get node name
     */
    const std::string& getNodeName() const { return node_name_; }
    
    /**
     * @brief Shortcut methods for each log level
     */
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void fatal(const std::string& msg);

private:
    void publish(const log_msg::LogEntry& entry);
    void outputLocal(const log_msg::LogEntry& entry);
    log_msg::LogEntry createEntry(LogLevel level, const std::string& message);
    
    std::string node_name_;
    LogLevel min_level_;
    bool local_output_;
    bool remote_output_;
    
    // Remote publish callback
    LogPublishCallback publish_callback_;
    
    uint32_t seq_;
    std::mutex mutex_;
};

/**
 * @brief Global Logger singleton (optional usage)
 */
class GlobalLogger {
public:
    static Logger& instance();
    static void init(const std::string& node_name, 
                     LogPublishCallback publish_callback = nullptr);
    static bool isInitialized();
    
private:
    static std::unique_ptr<Logger> logger_;
    static std::mutex mutex_;
    static bool initialized_;
};

} // namespace core

// ==================== Convenience Macros ====================

/**
 * @brief Stream-style logging macros
 * @example LOG_INFO(logger) << "value: " << 123;
 */
#define LOG_DEBUG(logger) (logger).stream(core::LogLevel::DEBUG)
#define LOG_INFO(logger)  (logger).stream(core::LogLevel::INFO)
#define LOG_WARN(logger)  (logger).stream(core::LogLevel::WARN)
#define LOG_ERROR(logger) (logger).stream(core::LogLevel::ERROR)
#define LOG_FATAL(logger) (logger).stream(core::LogLevel::FATAL)

/**
 * @brief Global Logger macros
 * @example GLOG_INFO << "value: " << 123;
 */
#define GLOG_DEBUG core::GlobalLogger::instance().stream(core::LogLevel::DEBUG)
#define GLOG_INFO  core::GlobalLogger::instance().stream(core::LogLevel::INFO)
#define GLOG_WARN  core::GlobalLogger::instance().stream(core::LogLevel::WARN)
#define GLOG_ERROR core::GlobalLogger::instance().stream(core::LogLevel::ERROR)
#define GLOG_FATAL core::GlobalLogger::instance().stream(core::LogLevel::FATAL)

/**
 * @brief Conditional logging macros
 * @example LOG_INFO_IF(logger, x > 0) << "x is positive";
 */
#define LOG_DEBUG_IF(logger, cond) if(cond) LOG_DEBUG(logger)
#define LOG_INFO_IF(logger, cond)  if(cond) LOG_INFO(logger)
#define LOG_WARN_IF(logger, cond)  if(cond) LOG_WARN(logger)
#define LOG_ERROR_IF(logger, cond) if(cond) LOG_ERROR(logger)
#define LOG_FATAL_IF(logger, cond) if(cond) LOG_FATAL(logger)

/**
 * @brief Throttled logging macros (log every N occurrences)
 * @example LOG_INFO_EVERY_N(logger, 100) << "periodic log";
 */
#define LOG_EVERY_N_IMPL(logger, level, n, counter) \
    static int counter = 0; \
    if (++counter % (n) == 0) \
        (logger).stream(level)

#define LOG_DEBUG_EVERY_N(logger, n) LOG_EVERY_N_IMPL(logger, core::LogLevel::DEBUG, n, LOG_COUNTER_##__LINE__)
#define LOG_INFO_EVERY_N(logger, n)  LOG_EVERY_N_IMPL(logger, core::LogLevel::INFO, n, LOG_COUNTER_##__LINE__)
#define LOG_WARN_EVERY_N(logger, n)  LOG_EVERY_N_IMPL(logger, core::LogLevel::WARN, n, LOG_COUNTER_##__LINE__)
#define LOG_ERROR_EVERY_N(logger, n) LOG_EVERY_N_IMPL(logger, core::LogLevel::ERROR, n, LOG_COUNTER_##__LINE__)

/**
 * @brief Log only once (first occurrence)
 * @example LOG_WARN_ONCE(logger) << "This warning appears only once";
 */
#define LOG_ONCE_IMPL(logger, level, flag) \
    static bool flag = false; \
    if (!flag && (flag = true)) \
        (logger).stream(level)

#define LOG_DEBUG_ONCE(logger) LOG_ONCE_IMPL(logger, core::LogLevel::DEBUG, LOG_ONCE_FLAG_##__LINE__)
#define LOG_INFO_ONCE(logger)  LOG_ONCE_IMPL(logger, core::LogLevel::INFO, LOG_ONCE_FLAG_##__LINE__)
#define LOG_WARN_ONCE(logger)  LOG_ONCE_IMPL(logger, core::LogLevel::WARN, LOG_ONCE_FLAG_##__LINE__)
#define LOG_ERROR_ONCE(logger) LOG_ONCE_IMPL(logger, core::LogLevel::ERROR, LOG_ONCE_FLAG_##__LINE__)
#define LOG_FATAL_ONCE(logger) LOG_ONCE_IMPL(logger, core::LogLevel::FATAL, LOG_ONCE_FLAG_##__LINE__)

/**
 * @brief Log only when condition changes from false to true
 * @example LOG_WARN_CHANGED(logger, voltage < 10.0) << "Low voltage!";
 * This will log only when voltage drops below 10.0, not continuously
 */
#define LOG_CHANGED_IMPL(logger, level, cond, prev_state) \
    static bool prev_state = false; \
    bool _curr_state = (cond); \
    if (_curr_state && !prev_state) \
        (logger).stream(level); \
    prev_state = _curr_state; \
    if (_curr_state && !prev_state)

#define LOG_DEBUG_CHANGED(logger, cond) LOG_CHANGED_IMPL(logger, core::LogLevel::DEBUG, cond, LOG_PREV_##__LINE__)
#define LOG_INFO_CHANGED(logger, cond)  LOG_CHANGED_IMPL(logger, core::LogLevel::INFO, cond, LOG_PREV_##__LINE__)
#define LOG_WARN_CHANGED(logger, cond)  LOG_CHANGED_IMPL(logger, core::LogLevel::WARN, cond, LOG_PREV_##__LINE__)
#define LOG_ERROR_CHANGED(logger, cond) LOG_CHANGED_IMPL(logger, core::LogLevel::ERROR, cond, LOG_PREV_##__LINE__)
#define LOG_FATAL_CHANGED(logger, cond) LOG_CHANGED_IMPL(logger, core::LogLevel::FATAL, cond, LOG_PREV_##__LINE__)

/**
 * @brief Log at most once per time interval (in milliseconds)
 * @example LOG_WARN_THROTTLE(logger, 1000) << "This logs at most once per second";
 */
#define LOG_THROTTLE_IMPL(logger, level, interval_ms, last_time) \
    static auto last_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(interval_ms); \
    auto _now = std::chrono::steady_clock::now(); \
    if (std::chrono::duration_cast<std::chrono::milliseconds>(_now - last_time).count() >= interval_ms) { \
        last_time = _now; \
        (logger).stream(level)

#define LOG_THROTTLE_END }

#define LOG_DEBUG_THROTTLE(logger, ms) LOG_THROTTLE_IMPL(logger, core::LogLevel::DEBUG, ms, LOG_THROTTLE_##__LINE__)
#define LOG_INFO_THROTTLE(logger, ms)  LOG_THROTTLE_IMPL(logger, core::LogLevel::INFO, ms, LOG_THROTTLE_##__LINE__)
#define LOG_WARN_THROTTLE(logger, ms)  LOG_THROTTLE_IMPL(logger, core::LogLevel::WARN, ms, LOG_THROTTLE_##__LINE__)
#define LOG_ERROR_THROTTLE(logger, ms) LOG_THROTTLE_IMPL(logger, core::LogLevel::ERROR, ms, LOG_THROTTLE_##__LINE__)
#define LOG_FATAL_THROTTLE(logger, ms) LOG_THROTTLE_IMPL(logger, core::LogLevel::FATAL, ms, LOG_THROTTLE_##__LINE__)

#endif // LOGGER_H

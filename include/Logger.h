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
class GlobalLogStream;

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
 * @brief Global Logger Implementation (Singleton)
 * 
 * Simplified logging system - just include Logger.h and use LOG_* macros anywhere.
 * All logs are published to /log topic via callback.
 * 
 * Features:
 * - Zero configuration required in other files
 * - Single initialization in main.cpp
 * - Thread-safe
 * - Auto file/line info
 * - Publishes to /log topic via user-provided callback
 */
class GlobalLoggerImpl {
public:
    /**
     * @brief Get singleton instance
     */
    static GlobalLoggerImpl& instance();
    
    /**
     * @brief Initialize logger with node name only (local output only)
     * @param node_name Name of the node for log identification
     * 
     * Use this for local-only logging. For remote publishing,
     * call setPublishCallback() after this.
     */
    static void init(const std::string& node_name);
    
    /**
     * @brief Set publish callback for remote logging
     * Call this after init() to enable remote publishing to /log topic
     */
    void setPublishCallback(LogPublishCallback callback);
    
    /**
     * @brief Set minimum log level
     */
    void setMinLevel(LogLevel level);
    
    /**
     * @brief Get minimum log level
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
     * @brief Log a message
     */
    void log(LogLevel level, const std::string& message, 
             const char* file = nullptr, int line = 0);
    
    /**
     * @brief Get node name
     */
    const std::string& getNodeName() const { return node_name_; }
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const { return initialized_; }

private:
    GlobalLoggerImpl();
    ~GlobalLoggerImpl() = default;
    
    // Disable copy
    GlobalLoggerImpl(const GlobalLoggerImpl&) = delete;
    GlobalLoggerImpl& operator=(const GlobalLoggerImpl&) = delete;
    
    void publish(const log_msg::LogEntry& entry);
    void outputLocal(const log_msg::LogEntry& entry);
    log_msg::LogEntry createEntry(LogLevel level, const std::string& message,
                                   const char* file, int line);
    
    std::string node_name_;
    LogLevel min_level_;
    bool local_output_;
    bool remote_output_;
    bool initialized_;
    
    LogPublishCallback publish_callback_;
    
    uint32_t seq_;
    std::mutex mutex_;
};

/**
 * @brief Log stream for global logger (stream-style API)
 * 
 * Used internally by LOG_* macros.
 * Automatically flushes on destruction.
 */
class GlobalLogStream {
public:
    GlobalLogStream(LogLevel level, const char* file, int line);
    ~GlobalLogStream();
    
    // Disable copy
    GlobalLogStream(const GlobalLogStream&) = delete;
    GlobalLogStream& operator=(const GlobalLogStream&) = delete;
    
    // Enable move
    GlobalLogStream(GlobalLogStream&& other) noexcept;
    
    // Stream operator
    template<typename T>
    GlobalLogStream& operator<<(const T& value) {
        if (active_) {
            ss_ << value;
        }
        return *this;
    }
    
private:
    LogLevel level_;
    const char* file_;
    int line_;
    std::ostringstream ss_;
    bool active_;
};

} // namespace core

// ==================== Simplified Global Macros ====================

/**
 * @brief Initialize global logger (call once in main.cpp)
 * @param name Node name for log identification
 * 
 * Example: LOG_INIT("fpga_driver");
 */
#define LOG_INIT(name) core::GlobalLoggerImpl::init(name)

/**
 * @brief Stream-style logging macros
 * Use these anywhere after including Logger.h - no other setup needed!
 * 
 * Example:
 *   LOG_INFO << "Hello world: " << 123;
 *   LOG_WARN << "Temperature: " << temp << "°C";
 *   LOG_ERROR << "Error code: " << err;
 */
#define LOG_DEBUG core::GlobalLogStream(core::LogLevel::DEBUG, __FILE__, __LINE__)
#define LOG_INFO  core::GlobalLogStream(core::LogLevel::INFO, __FILE__, __LINE__)
#define LOG_WARN  core::GlobalLogStream(core::LogLevel::WARN, __FILE__, __LINE__)
#define LOG_ERROR core::GlobalLogStream(core::LogLevel::ERROR, __FILE__, __LINE__)
#define LOG_FATAL core::GlobalLogStream(core::LogLevel::FATAL, __FILE__, __LINE__)

/**
 * @brief Conditional logging macros
 * Log only when condition is true
 * 
 * Example: LOG_INFO_IF(x > 0) << "x is positive: " << x;
 */
#define LOG_DEBUG_IF(cond) if(cond) LOG_DEBUG
#define LOG_INFO_IF(cond)  if(cond) LOG_INFO
#define LOG_WARN_IF(cond)  if(cond) LOG_WARN
#define LOG_ERROR_IF(cond) if(cond) LOG_ERROR
#define LOG_FATAL_IF(cond) if(cond) LOG_FATAL

/**
 * @brief Log only once (first occurrence)
 * 
 * Example: LOG_WARN_ONCE << "This warning appears only once";
 */
#define LOG_ONCE_IMPL(level, flag) \
    static bool flag = false; \
    if (!flag && (flag = true)) \
        core::GlobalLogStream(level, __FILE__, __LINE__)

#define LOG_DEBUG_ONCE LOG_ONCE_IMPL(core::LogLevel::DEBUG, LOG_ONCE_FLAG_##__LINE__)
#define LOG_INFO_ONCE  LOG_ONCE_IMPL(core::LogLevel::INFO, LOG_ONCE_FLAG_##__LINE__)
#define LOG_WARN_ONCE  LOG_ONCE_IMPL(core::LogLevel::WARN, LOG_ONCE_FLAG_##__LINE__)
#define LOG_ERROR_ONCE LOG_ONCE_IMPL(core::LogLevel::ERROR, LOG_ONCE_FLAG_##__LINE__)
#define LOG_FATAL_ONCE LOG_ONCE_IMPL(core::LogLevel::FATAL, LOG_ONCE_FLAG_##__LINE__)

/**
 * @brief Log every N occurrences
 * 
 * Example: LOG_INFO_EVERY_N(100) << "Processed " << count << " items";
 */
#define LOG_EVERY_N_IMPL(level, n, counter) \
    static int counter = 0; \
    if (++counter % (n) == 0) \
        core::GlobalLogStream(level, __FILE__, __LINE__)

#define LOG_DEBUG_EVERY_N(n) LOG_EVERY_N_IMPL(core::LogLevel::DEBUG, n, LOG_COUNTER_##__LINE__)
#define LOG_INFO_EVERY_N(n)  LOG_EVERY_N_IMPL(core::LogLevel::INFO, n, LOG_COUNTER_##__LINE__)
#define LOG_WARN_EVERY_N(n)  LOG_EVERY_N_IMPL(core::LogLevel::WARN, n, LOG_COUNTER_##__LINE__)
#define LOG_ERROR_EVERY_N(n) LOG_EVERY_N_IMPL(core::LogLevel::ERROR, n, LOG_COUNTER_##__LINE__)

/**
 * @brief Log when condition changes from false to true
 * 
 * Example: LOG_WARN_CHANGED(voltage < 10.0) << "Low voltage!";
 */
#define LOG_CHANGED_IMPL(level, cond, prev_state) \
    static bool prev_state = false; \
    bool _curr_state_##__LINE__ = (cond); \
    if (_curr_state_##__LINE__ && !prev_state) \
        core::GlobalLogStream(level, __FILE__, __LINE__); \
    prev_state = _curr_state_##__LINE__; \
    if (_curr_state_##__LINE__ && !prev_state)

#define LOG_DEBUG_CHANGED(cond) LOG_CHANGED_IMPL(core::LogLevel::DEBUG, cond, LOG_PREV_##__LINE__)
#define LOG_INFO_CHANGED(cond)  LOG_CHANGED_IMPL(core::LogLevel::INFO, cond, LOG_PREV_##__LINE__)
#define LOG_WARN_CHANGED(cond)  LOG_CHANGED_IMPL(core::LogLevel::WARN, cond, LOG_PREV_##__LINE__)
#define LOG_ERROR_CHANGED(cond) LOG_CHANGED_IMPL(core::LogLevel::ERROR, cond, LOG_PREV_##__LINE__)
#define LOG_FATAL_CHANGED(cond) LOG_CHANGED_IMPL(core::LogLevel::FATAL, cond, LOG_PREV_##__LINE__)

/**
 * @brief Log at most once per time interval (in milliseconds)
 * 
 * Example: 
 *   LOG_WARN_THROTTLE(1000) << "This logs at most once per second";
 *   LOG_THROTTLE_END
 */
#define LOG_THROTTLE_IMPL(level, interval_ms, last_time) \
    static auto last_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(interval_ms); \
    auto _now_##__LINE__ = std::chrono::steady_clock::now(); \
    if (std::chrono::duration_cast<std::chrono::milliseconds>(_now_##__LINE__ - last_time).count() >= interval_ms) { \
        last_time = _now_##__LINE__; \
        core::GlobalLogStream(level, __FILE__, __LINE__)

#define LOG_THROTTLE_END }

#define LOG_DEBUG_THROTTLE(ms) LOG_THROTTLE_IMPL(core::LogLevel::DEBUG, ms, LOG_THROTTLE_##__LINE__)
#define LOG_INFO_THROTTLE(ms)  LOG_THROTTLE_IMPL(core::LogLevel::INFO, ms, LOG_THROTTLE_##__LINE__)
#define LOG_WARN_THROTTLE(ms)  LOG_THROTTLE_IMPL(core::LogLevel::WARN, ms, LOG_THROTTLE_##__LINE__)
#define LOG_ERROR_THROTTLE(ms) LOG_THROTTLE_IMPL(core::LogLevel::ERROR, ms, LOG_THROTTLE_##__LINE__)
#define LOG_FATAL_THROTTLE(ms) LOG_THROTTLE_IMPL(core::LogLevel::FATAL, ms, LOG_THROTTLE_##__LINE__)

#endif // LOGGER_H

#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>

namespace core {

// ==================== ANSI Color Codes ====================
namespace {
    const char* RESET   = "\033[0m";
    const char* RED     = "\033[31m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* MAGENTA = "\033[35m";
    const char* CYAN    = "\033[36m";
    const char* WHITE   = "\033[37m";
    const char* BOLD    = "\033[1m";

    const char* getLevelColor(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return CYAN;
            case LogLevel::INFO:  return GREEN;
            case LogLevel::WARN:  return YELLOW;
            case LogLevel::ERROR: return RED;
            case LogLevel::FATAL: return MAGENTA;
            default:              return WHITE;
        }
    }
}

// ==================== LogStream Implementation ====================

LogStream::LogStream(Logger& logger, LogLevel level)
    : logger_(logger)
    , level_(level)
    , active_(level >= logger.getMinLevel())
{
}

LogStream::~LogStream() {
    if (active_ && !ss_.str().empty()) {
        logger_.log(level_, ss_.str());
    }
}

LogStream::LogStream(LogStream&& other) noexcept
    : logger_(other.logger_)
    , level_(other.level_)
    , ss_(std::move(other.ss_))
    , active_(other.active_)
{
    other.active_ = false;
}

// ==================== Logger Implementation ====================

Logger::Logger(const std::string& node_name, 
               LogPublishCallback publish_callback)
    : node_name_(node_name)
    , min_level_(LogLevel::DEBUG)
    , local_output_(true)
    , remote_output_(publish_callback != nullptr)
    , publish_callback_(publish_callback)
    , seq_(0)
{
}

void Logger::setPublishCallback(LogPublishCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    publish_callback_ = callback;
    remote_output_ = (callback != nullptr);
}

void Logger::setMinLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

LogLevel Logger::getMinLevel() const {
    return min_level_;
}

void Logger::setLocalOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    local_output_ = enabled;
}

void Logger::setRemoteOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    remote_output_ = enabled;
}

log_msg::LogEntry Logger::createEntry(LogLevel level, const std::string& message) {
    log_msg::LogEntry entry;
    
    // Set Header with timestamp
    auto* header = entry.mutable_header();
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    header->mutable_stamp()->set_sec(tv.tv_sec);
    header->mutable_stamp()->set_usec(tv.tv_usec);
    header->set_seq(seq_++);
    header->set_frameid(node_name_);
    
    // Set log content
    entry.set_level(toProtoLevel(level));
    entry.set_node_name(node_name_);
    entry.set_message(message);
    
    return entry;
}

void Logger::outputLocal(const log_msg::LogEntry& entry) {
    // Format timestamp
    time_t sec = entry.header().stamp().sec();
    struct tm* tm_info = localtime(&sec);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
    
    // Get level color and string
    LogLevel level = static_cast<LogLevel>(entry.level());
    const char* color = getLevelColor(level);
    const char* level_str = logLevelToString(level);
    
    // Output format: [TIME.USEC] [LEVEL] [NODE] message
    std::cerr << "[" << time_buf << "." 
              << std::setfill('0') << std::setw(6) << entry.header().stamp().usec() << "] "
              << color << BOLD << "[" << std::setw(5) << level_str << "]" << RESET << " "
              << "[" << entry.node_name() << "] "
              << entry.message()
              << std::endl;
}

void Logger::publish(const log_msg::LogEntry& entry) {
    if (publish_callback_) {
        publish_callback_(entry);
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < min_level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    log_msg::LogEntry entry = createEntry(level, message);
    
    // Local console output
    if (local_output_) {
        outputLocal(entry);
    }
    
    // Remote publish via callback
    if (remote_output_ && publish_callback_) {
        publish(entry);
    }
}

LogStream Logger::stream(LogLevel level) {
    return LogStream(*this, level);
}

// Shortcut methods
void Logger::debug(const std::string& msg) {
    log(LogLevel::DEBUG, msg);
}

void Logger::info(const std::string& msg) {
    log(LogLevel::INFO, msg);
}

void Logger::warn(const std::string& msg) {
    log(LogLevel::WARN, msg);
}

void Logger::error(const std::string& msg) {
    log(LogLevel::ERROR, msg);
}

void Logger::fatal(const std::string& msg) {
    log(LogLevel::FATAL, msg);
}

// ==================== GlobalLogger Implementation ====================

std::unique_ptr<Logger> GlobalLogger::logger_;
std::mutex GlobalLogger::mutex_;
bool GlobalLogger::initialized_ = false;

Logger& GlobalLogger::instance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!logger_) {
        // Create default logger with local output only
        logger_ = std::make_unique<Logger>("default");
    }
    return *logger_;
}

void GlobalLogger::init(const std::string& node_name, 
                        LogPublishCallback publish_callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    logger_ = std::make_unique<Logger>(node_name, publish_callback);
    initialized_ = true;
}

bool GlobalLogger::isInitialized() {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

} // namespace core

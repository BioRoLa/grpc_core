#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <libgen.h>  // For basename()
#include <cstring>   // For strdup()

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
    
    // Extract filename from full path
    std::string getFileName(const char* path) {
        if (!path) return "";
        char* pathCopy = strdup(path);
        std::string filename = basename(pathCopy);
        free(pathCopy);
        return filename;
    }
}

// ==================== GlobalLoggerImpl Implementation ====================

GlobalLoggerImpl::GlobalLoggerImpl()
    : node_name_("unknown")
    , min_level_(LogLevel::DEBUG)
    , local_output_(true)
    , remote_output_(false)
    , initialized_(false)
    , seq_(0)
{
}

GlobalLoggerImpl& GlobalLoggerImpl::instance() {
    static GlobalLoggerImpl instance;
    return instance;
}

void GlobalLoggerImpl::init(const std::string& node_name) {
    auto& impl = instance();
    std::lock_guard<std::mutex> lock(impl.mutex_);
    impl.node_name_ = node_name;
    impl.initialized_ = true;
}

void GlobalLoggerImpl::setPublishCallback(LogPublishCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    publish_callback_ = callback;
    remote_output_ = (callback != nullptr);
}

void GlobalLoggerImpl::setMinLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

LogLevel GlobalLoggerImpl::getMinLevel() const {
    return min_level_;
}

void GlobalLoggerImpl::setLocalOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    local_output_ = enabled;
}

void GlobalLoggerImpl::setRemoteOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    remote_output_ = enabled;
}

log_msg::LogEntry GlobalLoggerImpl::createEntry(LogLevel level, const std::string& message,
                                                  const char* file, int line) {
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
    
    // Include file:line info if available
    if (file && line > 0) {
        std::string filename = getFileName(file);
        entry.set_message("[" + filename + ":" + std::to_string(line) + "] " + message);
    } else {
        entry.set_message(message);
    }
    
    return entry;
}

void GlobalLoggerImpl::outputLocal(const log_msg::LogEntry& entry) {
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
              << color << BOLD << "["
              << std::setfill(' ') << std::left << std::setw(5) << level_str 
              << "]" << RESET << " "
              << "[" << entry.node_name() << "] "
              << entry.message()
              << std::endl;
}

void GlobalLoggerImpl::publish(const log_msg::LogEntry& entry) {
    if (publish_callback_) {
        publish_callback_(entry);
    }
}

void GlobalLoggerImpl::log(LogLevel level, const std::string& message,
                            const char* file, int line) {
    if (level < min_level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    log_msg::LogEntry entry = createEntry(level, message, file, line);
    
    // Local console output
    if (local_output_) {
        outputLocal(entry);
    }
    
    // Remote publish via callback
    if (remote_output_ && publish_callback_) {
        publish(entry);
    }
}

// ==================== GlobalLogStream Implementation ====================

GlobalLogStream::GlobalLogStream(LogLevel level, const char* file, int line)
    : level_(level)
    , file_(file)
    , line_(line)
    , active_(level >= GlobalLoggerImpl::instance().getMinLevel())
{
}

GlobalLogStream::~GlobalLogStream() {
    if (active_ && !ss_.str().empty()) {
        GlobalLoggerImpl::instance().log(level_, ss_.str(), file_, line_);
    }
}

GlobalLogStream::GlobalLogStream(GlobalLogStream&& other) noexcept
    : level_(other.level_)
    , file_(other.file_)
    , line_(other.line_)
    , ss_(std::move(other.ss_))
    , active_(other.active_)
{
    other.active_ = false;
}

} // namespace core

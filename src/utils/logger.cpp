#include "utils/logger.hpp"
#include <iomanip>
#include <sstream>

namespace plog  {

// ========================================
// 함수형 로거 구현
// ========================================

static std::string getCurrentTimestamp() {
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << t->tm_hour << ":"
        << std::setw(2) << t->tm_min << ":"
        << std::setw(2) << t->tm_sec;
    return oss.str();
}

void log(const std::string& message) {
    std::cout << "[" << getCurrentTimestamp() << "] " << message << std::endl << std::flush;
}

void logInfo(const std::string& message) {
    std::cout << "[" << getCurrentTimestamp() << "][INFO] " << message << std::endl << std::flush;
}

void logWarning(const std::string& message) {
    std::cout << "[" << getCurrentTimestamp() << "][WARN] " << message << std::endl << std::flush;
}

void logError(const std::string& message) {
    std::cerr << "[" << getCurrentTimestamp() << "][ERROR] " << message << std::endl << std::flush;
}

void logFatal(const std::string& message) {
    std::cerr << "[" << getCurrentTimestamp() << "][FATAL] " << message << std::endl << std::flush;
}

// ========================================
// 클래스형 로거 구현
// ========================================

Logger::Logger(const std::string& name) : logger_name(name) {}

std::string Logger::getTimestamp() {
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << t->tm_hour << ":"
        << std::setw(2) << t->tm_min << ":"
        << std::setw(2) << t->tm_sec;
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

void Logger::print(LogLevel level, const std::string& msg) {
    std::ostream& out = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
    out << "[" << getTimestamp() << "]"
        << "[" << levelToString(level) << "]"
        << "[" << logger_name << "] "
        << msg << std::endl << std::flush;
}

void Logger::info(const std::string& msg) {
    print(LogLevel::INFO, msg);
}

void Logger::warning(const std::string& msg) {
    print(LogLevel::WARNING, msg);
}

void Logger::error(const std::string& msg) {
    print(LogLevel::ERROR, msg);
}

void Logger::fatal(const std::string& msg) {
    print(LogLevel::FATAL, msg);
}

void Logger::setName(const std::string& name) {
    logger_name = name;
}

std::string Logger::getName() const {
    return logger_name;
}

} // namespace utils

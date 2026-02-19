#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <ctime>

namespace plog {

// 로그 레벨
enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// 간단한 함수형 로거
void log(const std::string& message);
void logInfo(const std::string& message);
void logWarning(const std::string& message);
void logError(const std::string& message);
void logFatal(const std::string& message);

// 클래스형 로거
class Logger {
public:
    Logger(const std::string& name);
    
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);
    void fatal(const std::string& msg);
    
    void setName(const std::string& name);
    std::string getName() const;

private:
    std::string logger_name;
    
    std::string getTimestamp();
    void print(LogLevel level, const std::string& msg);
    std::string levelToString(LogLevel level);
};

} // namespace utils

#endif // LOGGER_HPP

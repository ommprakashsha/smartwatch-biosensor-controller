#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <mutex>
#include <fstream>
#include <iostream>

enum class LogLevel {
    INFO,
    HEALTH_ALERT,
    HAPTIC_EVENT,
    ERROR
};

class Logger {
public:
    static Logger& getInstance();

    void init(const std::string& logFilePath = "smartwatch_health.log");
    void log(LogLevel level, const std::string& message);

    void info(const std::string& message) { log(LogLevel::INFO, message); }
    void alert(const std::string& message) { log(LogLevel::HEALTH_ALERT, message); }
    void haptic(const std::string& message) { log(LogLevel::HAPTIC_EVENT, message); }
    void error(const std::string& message) { log(LogLevel::ERROR, message); }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string getCurrentTimestamp();
    std::string levelToString(LogLevel level);

    std::mutex logMutex_;
    std::ofstream fileStream_;
    bool isFileOpen_;
};

#endif // LOGGER_HPP

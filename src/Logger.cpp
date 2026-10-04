#include "Logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : isFileOpen_(false) {}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (fileStream_.is_open()) fileStream_.close();
}

void Logger::init(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(logMutex_);
    fileStream_.open(logFilePath, std::ios::out | std::ios::app);
    isFileOpen_ = fileStream_.is_open();
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:         return "INFO";
        case LogLevel::HEALTH_ALERT: return "HEART_ALERT";
        case LogLevel::HAPTIC_EVENT: return "HAPTIC_EVENT";
        case LogLevel::ERROR:        return "ERROR";
        default:                     return "LOG";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex_);
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = levelToString(level);

    std::string colorCode = "\033[0m";
    if (level == LogLevel::HEALTH_ALERT) colorCode = "\033[1;31m";  // Bold Red
    else if (level == LogLevel::HAPTIC_EVENT) colorCode = "\033[1;36m"; // Bold Cyan
    else if (level == LogLevel::INFO) colorCode = "\033[1;32m";  // Bold Green
    else if (level == LogLevel::ERROR) colorCode = "\033[1;35m"; // Magenta

    std::string formattedMsg = "[" + timestamp + "] [" + levelStr + "] " + message;
    std::cout << colorCode << formattedMsg << "\033[0m" << std::endl;

    if (isFileOpen_) {
        fileStream_ << formattedMsg << std::endl;
        fileStream_.flush();
    }
}

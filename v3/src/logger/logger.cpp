//src/logger/logger.cpp
#include "lightdb/logger.h"  // 正确路径
#include <ctime>
#include <exception>
#include <iomanip>

namespace lightdb {
    Logger& Logger::GetInstance() {
        static Logger instance;
        return instance;
    }

    void Logger::SetLogLevel(LogLevel level) {
        current_level_ = level;
    }

    std::string Logger::GetLogPrefix(LogLevel level) {
        auto now = std::time(nullptr);
        auto tm = std::localtime(&now);
        char time_buf[64];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm);

        std::string level_str;
        switch(level) {
            case LogLevel::DEBUG:level_str = "[DEBUG]"; break;
            case LogLevel::INFO: level_str = "[INFO ]"; break;
            case LogLevel::WARN: level_str = "[WARN ]"; break;
            case LogLevel::ERROR:level_str = "[ERROR]"; break;
        }
        return std::string(time_buf) + " " + level_str + " ";
    }

    void Logger::Debug(const std::string& msg) {
        if(current_level_ <= LogLevel::DEBUG) {
            std::cout<<GetLogPrefix(LogLevel::DEBUG) << msg << std::endl;
        }
    }

    void Logger::Info(const std::string& msg) {
        if(current_level_ <= LogLevel::INFO) {
            std::cout<<GetLogPrefix(LogLevel::INFO) << msg << std::endl;
        }
    }

    void Logger::Warn(const std::string& msg) {
        if(current_level_ <= LogLevel::WARN) {
            std::cout<<GetLogPrefix(LogLevel::WARN) << msg << std::endl;
        }
    }

    void Logger::Error(const std::string& msg) {
        if(current_level_ <= LogLevel::ERROR) {
            std::cerr<<GetLogPrefix(LogLevel::ERROR) << msg << std::endl;
        }
    }
}

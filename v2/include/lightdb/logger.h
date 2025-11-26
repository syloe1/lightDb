// include/lightdb/logger.h
#ifndef LIGHTDB_LOGGER_H
#define LIGHTDB_LOGGER_H

#include <string>
#include <iostream>
#include <ctime>

namespace lightdb {
    enum class LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR 
    };
    class Logger {
        public:
            static Logger& GetInstance();

            void SetLogLevel(LogLevel level);

            void Debug(const std::string& msg);
            void Info(const std::string& msg);
            void Warn(const std::string& msg);
            void Error(const std::string& msg);
        private:
            Logger() = default;
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;

            std::string GetLogPrefix(LogLevel level);

            LogLevel current_level_ = LogLevel::INFO;
    };
    #define LOG_DEBUG(msg) lightdb::Logger::GetInstance().Debug(msg);
    #define LOG_INFO(msg) lightdb::Logger::GetInstance().Info(msg);
    #define LOG_WARN(msg) lightdb::Logger::GetInstance().Warn(msg);
    #define LOG_ERROR(msg) lightdb::Logger::GetInstance().Error(msg);
}
#endif 
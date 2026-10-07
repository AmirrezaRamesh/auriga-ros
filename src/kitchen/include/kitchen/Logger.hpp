#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include "rclcpp/rclcpp.hpp"

class Logger
{
public:
    static rclcpp::Logger get_logger(const std::string &str);
    static rclcpp::Clock &get_clock();
};

// ============================================================
//  Plain logging (no throttle)
// ============================================================
#define LOG_DEBUG(name, ...) \
    RCLCPP_DEBUG(Logger::get_logger(name), __VA_ARGS__)

#define LOG_INFO(name, ...) \
    RCLCPP_INFO(Logger::get_logger(name), __VA_ARGS__)

#define LOG_WARN(name, ...) \
    RCLCPP_WARN(Logger::get_logger(name), __VA_ARGS__)

#define LOG_ERROR(name, ...) \
    RCLCPP_ERROR(Logger::get_logger(name), __VA_ARGS__)

#define LOG_FATAL(name, ...) \
    RCLCPP_FATAL(Logger::get_logger(name), __VA_ARGS__)

// ============================================================
//  Throttled logging
// ============================================================
#define LOG_DEBUG_THROTTLE(name, period_ms, ...) \
    RCLCPP_DEBUG_THROTTLE(Logger::get_logger(name), Logger::get_clock(), period_ms, __VA_ARGS__)

#define LOG_INFO_THROTTLE(name, period_ms, ...) \
    RCLCPP_INFO_THROTTLE(Logger::get_logger(name), Logger::get_clock(), period_ms, __VA_ARGS__)

#define LOG_WARN_THROTTLE(name, period_ms, ...) \
    RCLCPP_WARN_THROTTLE(Logger::get_logger(name), Logger::get_clock(), period_ms, __VA_ARGS__)

#define LOG_ERROR_THROTTLE(name, period_ms, ...) \
    RCLCPP_ERROR_THROTTLE(Logger::get_logger(name), Logger::get_clock(), period_ms, __VA_ARGS__)

#define LOG_FATAL_THROTTLE(name, period_ms, ...) \
    RCLCPP_FATAL_THROTTLE(Logger::get_logger(name), Logger::get_clock(), period_ms, __VA_ARGS__)

#endif // __LOGGER_HPP_
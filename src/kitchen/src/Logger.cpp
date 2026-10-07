#include "../include/kitchen/Logger.hpp"

rclcpp::Logger Logger::get_logger(const std::string &str)
{
    return rclcpp::get_logger(str);
}

rclcpp::Clock &Logger::get_clock()
{
    static rclcpp::Clock global_system_clock(RCL_SYSTEM_TIME);
    return global_system_clock;
}
#include "rclcpp/rclcpp.hpp"
#include "interface/msg/robot.hpp"

#include <memory>
#include <string>

#include "../include/kitchen/Logger.hpp"

class KithcenHandler
{
public:
    void set_robot_state(std::string state)
    {
        robot_state_ = state;
    }
    std::string get_robot_state()
    {
        return robot_state_;
    }

private:
    std::string robot_state_;
};

using std::placeholders::_1;
using Robot = interface::msg::Robot;

class KitchenNode : public rclcpp::Node
{
public:
    KitchenNode()
        : Node("kitchen_node")
    {
        // robot state subscriber
        robot_state_subscriber_ = this->create_subscription<Robot>(
            "robot_state",
            10,
            std::bind(&KitchenNode::topic_callback, this, _1));
    }

private:
    void topic_callback(const Robot::SharedPtr msg)
    {
        kithcenHandler.set_robot_state(msg->robot_state);
    }
    rclcpp::Subscription<Robot>::SharedPtr robot_state_subscriber_;
    KithcenHandler kithcenHandler;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<KitchenNode>());
    rclcpp::shutdown();
    return 0;
}
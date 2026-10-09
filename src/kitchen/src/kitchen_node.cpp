#include "rclcpp/rclcpp.hpp"
#include "interface/msg/robot.hpp"
#include "interface/srv/order.hpp"

#include <memory>
#include <string>
#include <deque>

#include "../include/kitchen/Logger.hpp"
#include "../include/kitchen/Menu.hpp"

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

    bool get_order(std::string name)
    {
        for (auto &m : menu)
        {
            if (m.name == name && m.remaining > 0)
            {
                orders.push_back(m); // add food to orders que
                m.remaining--;       // one of stock was used
                LOG_INFO("kitchen", "%s added to que !\n%i remains !", m.name.c_str(), m.remaining);
                return true; // response was successful
            }
        }
        LOG_INFO("kitchen", "there was a no successful order !!!");
        return false;
    }

private:
    std::string robot_state_;

    struct Order
    {
    public:
        Order(const Food &food_)
        {
            food = food_;
            remaining_time = food_.preparationTime;
            is_ready = false;
        }
        Food food;
        bool is_ready;
        int remaining_time;
    };

    std::deque<Order> orders = {};
};

using std::placeholders::_1;
using std::placeholders::_2;
using Robot = interface::msg::Robot;
using Order = interface::srv::Order;

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

        // order service
        order_service_ = this->create_service<Order>(
            "order",
            std::bind(&KitchenNode::order_Service_callback, this, _1, _2));
    }

private:
    void topic_callback(const Robot::SharedPtr msg)
    {
        kithcenHandler.set_robot_state(msg->robot_state);
    }

    void order_Service_callback(const std::shared_ptr<Order::Request> request,
                                std::shared_ptr<Order::Response> response)
    {
        response->success = kithcenHandler.get_order(request->name);
    }

    rclcpp::Subscription<Robot>::SharedPtr robot_state_subscriber_;
    rclcpp::Service<Order>::SharedPtr order_service_;

    // main controller for kitchen
    KithcenHandler kithcenHandler;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<KitchenNode>());
    rclcpp::shutdown();
    return 0;
}
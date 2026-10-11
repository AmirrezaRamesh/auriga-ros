#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <array>

using namespace std::chrono_literals;

class customer_node : public rclcpp::Node
{
public:

    customer_node() : Node("customer_node")
    {
        publisher_ = this->create_publisher<std_msgs::msg::String>("order", 10);

        timer_ = this->create_wall_timer(
            5s,
            std::bind(&customer_node::publish_order, this)
        );

        RCLCPP_INFO(this->get_logger(), "Customer node has started!");
    }

private:

    void publish_order()
    {
        std::array<std::string, 3> tables = {
            "Table 1", "Table 2", "Table 3"
        };

        std::array<std::string, 3> foods = {
            "Pizza", "Burger", "Pasta"
        };

        auto message = std_msgs::msg::String();

        message.data = tables[order_index_] +
                       " requests " +
                       foods[order_index_];

        publisher_->publish(message);

        RCLCPP_INFO(
            this->get_logger(),
            "Published order: %s",
            message.data.c_str()
        );

        order_index_ = (order_index_ + 1) % 3;
    }

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;

    int order_index_ = 0;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<customer_node>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
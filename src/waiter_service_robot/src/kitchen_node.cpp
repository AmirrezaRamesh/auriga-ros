#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <memory>

class kitchen_node : public rclcpp::Node{
    public:
        kitchen_node() : Node("kitchen_node")
    {
        subscription_ = this->create_subscription<std_msgs::msg::String>("order", 10, [this](std_msgs::msg::String::SharedPtr message){
                RCLCPP_INFO(this->get_logger(), "Kitchen received order: %s", message->data.c_str());
            }
        );

        RCLCPP_INFO(this->get_logger(), "Kitchen node has started! Waiting for orders...");
    }

private:
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<kitchen_node>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
```
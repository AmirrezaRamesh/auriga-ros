#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <chrono>
#include <memory>

using namespace std::chrono_literals;

class robot_node : public rclcpp::Node{
    public:
        robot_node() : Node("robot_node")
    {
        state_publisher_ = this->create_publisher<std_msgs::msg::String>("robot_state", 10);

        subscription_ = this->create_subscription<std_msgs::msg::String>("order", 10, [this](std_msgs::msg::String::SharedPtr message){
                RCLCPP_INFO(this->get_logger(), "Robot received order: %s", message->data.c_str());

                publish_state("BUSY");

                busy_timer_ = this->create_wall_timer(2s, [this](){
                    publish_state("FREE");
                    busy_timer_->cancel();
                });
            }
        );

        publish_state("FREE");

        RCLCPP_INFO(this->get_logger(), "Robot node has started! Waiting for orders...");
    }

private:

    void publish_state(const std::string & state)
    {
        auto message = std_msgs::msg::String();
        message.data = state;

        state_publisher_->publish(message);

        RCLCPP_INFO(this->get_logger(), "Robot state: %s", state.c_str());
    }

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr state_publisher_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
    rclcpp::TimerBase::SharedPtr busy_timer_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<robot_node>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}

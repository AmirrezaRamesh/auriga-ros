#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <chrono>
#include <memory>
#include <string>

using namespace std::chrono_literals;

class robot_node : public rclcpp::Node{
    public:
        robot_node() : Node("robot_node")
    {
        state_publisher_ = this->create_publisher<std_msgs::msg::String>("robot_state", 10);

        order_subscription_ = this->create_subscription<std_msgs::msg::String>("order", 10, [this](std_msgs::msg::String::SharedPtr message){
                RCLCPP_INFO(this->get_logger(), "Robot received order: %s", message->data.c_str());

                publish_state("BUSY");
                start_busy_timer();
            }
        );

        food_subscription_ = this->create_subscription<std_msgs::msg::String>("food_ready", 10, [this](std_msgs::msg::String::SharedPtr message){
                RCLCPP_INFO(this->get_logger(), "Robot received food: %s", message->data.c_str());
                RCLCPP_INFO(this->get_logger(), "Delivering food to the customer!");

                publish_state("BUSY");
                start_busy_timer();
            }
        );

        publish_state("FREE");

        RCLCPP_INFO(this->get_logger(), "Robot node started! Waiting for orders...");
    }

private:

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr state_publisher_;

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr order_subscription_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr food_subscription_;

    rclcpp::TimerBase::SharedPtr busy_timer_;

    void publish_state(const std::string & state)
    {
        auto message = std_msgs::msg::String();
        message.data = state;

        state_publisher_->publish(message);

        RCLCPP_INFO(this->get_logger(), "Robot state: %s", state.c_str());
    }

    void start_busy_timer()
    {
        if (busy_timer_)
            busy_timer_->cancel();

        busy_timer_ = this->create_wall_timer(2s, [this](){
            publish_state("FREE");
            busy_timer_->cancel();
        });
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<robot_node>();

    rclcpp::spin(node);

    rclcpp::shutdown();

    return 0;
}
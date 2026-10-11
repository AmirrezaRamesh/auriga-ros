#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"

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
        publisher_ =
            this->create_publisher<std_msgs::msg::String>(
                "order", 10);

        food_available_client_ =
            this->create_client<std_srvs::srv::Trigger>(
                "food_available");

        timer_ = this->create_wall_timer(
            5s,
            std::bind(&customer_node::publish_order, this));

        RCLCPP_INFO(
            this->get_logger(),
            "Customer node has started!");
    }

private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;

    rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr
        food_available_client_;

    rclcpp::TimerBase::SharedPtr timer_;

    std::array<std::string, 3> tables_ = {
        "Table 1", "Table 2", "Table 3"
    };

    std::array<std::string, 3> foods_ = {
        "Pizza", "Burger", "Pasta"
    };
    int order_index_ = 0;
    void publish_order()
    {
        if (!food_available_client_->service_is_ready()) {
            RCLCPP_WARN(
                this->get_logger(),
                "Kitchen service is unavailable. No order sent.");
            return;
        }

        auto request =
            std::make_shared<std_srvs::srv::Trigger::Request>();

        food_available_client_->async_send_request(
            request,
            [this](rclcpp::Client<std_srvs::srv::Trigger>::SharedFuture future)
            {
                auto response = future.get();

                if (!response->success) {
                    timer_->cancel();
                    RCLCPP_WARN(this->get_logger(),"Restaurant is closed. Customer stopped ordering.");
                    return;
            }

                auto message = std_msgs::msg::String();

                message.data =
                    tables_[order_index_] +
                    " requests " +
                    foods_[order_index_];

                publisher_->publish(message);

                RCLCPP_INFO(
                    this->get_logger(),
                    "Published order: %s",
                    message.data.c_str());

                order_index_ = (order_index_ + 1) % 3;
            });
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<customer_node>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}

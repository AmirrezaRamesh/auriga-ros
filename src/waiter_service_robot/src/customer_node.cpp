#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"

#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <string>

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
    rclcpp::TimerBase::SharedPtr order_timer_; 

    std::array<std::string, 3> tables_ = {
        "Table 1", "Table 2", "Table 3"
    };

    std::array<std::string, 3> foods_ = {
        "Pizza", "Burger", "Pasta"
    };

    std::mt19937 rng_{std::random_device{}()};
    std::uniform_int_distribution<int> index_dist_{0, 2};
    std::uniform_real_distribution<double> delay_dist_{0.2, 0.9};

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
                    RCLCPP_WARN(
                        this->get_logger(),
                        "Restaurant is closed. Customer stopped ordering.");
                    return;
                }

                std::string order =
                    tables_[index_dist_(rng_)] +
                    " requests " +
                    foods_[index_dist_(rng_)];

                auto delay =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::duration<double>(delay_dist_(rng_)));

                order_timer_ = this->create_wall_timer(
                    delay,
                    [this, order]()
                    {
                        order_timer_->cancel();

                        auto message = std_msgs::msg::String();
                        message.data = order;
                        publisher_->publish(message);

                        RCLCPP_INFO(
                            this->get_logger(),
                            "Published order: %s",
                            message.data.c_str());
                    });
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
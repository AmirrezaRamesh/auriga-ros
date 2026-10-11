#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <chrono>
#include <deque>
#include <memory>
#include <string>

using namespace std::chrono_literals;

class robot_node : public rclcpp::Node
{
public:
    robot_node() : Node("robot_node")
    {
        state_publisher_ =
            this->create_publisher<std_msgs::msg::String>(
                "robot_state", 10);

        kitchen_order_publisher_ =
            this->create_publisher<std_msgs::msg::String>(
                "kitchen_order", 10);

        order_subscription_ =
            this->create_subscription<std_msgs::msg::String>(
                "order", 10,
                [this](std_msgs::msg::String::SharedPtr message)
                {
                    order_queue_.push_back(message->data);

                    RCLCPP_INFO(
                        this->get_logger(),
                        "Order added to queue: %s",
                        message->data.c_str());

                    process_next_task();
                });

        food_subscription_ =
            this->create_subscription<std_msgs::msg::String>(
                "food_ready", 10,
                [this](std_msgs::msg::String::SharedPtr message)
                {
                    food_queue_.push_back(message->data);

                    RCLCPP_INFO(
                        this->get_logger(),
                        "Food added to delivery queue: %s",
                        message->data.c_str());

                    process_next_task();
                });

        publish_state("FREE");

        RCLCPP_INFO(
            this->get_logger(),
            "Robot started and waiting for tasks.");
    }

private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr state_publisher_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr kitchen_order_publisher_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr order_subscription_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr food_subscription_;

    rclcpp::TimerBase::SharedPtr task_timer_;

    std::deque<std::string> order_queue_;
    std::deque<std::string> food_queue_;

    bool robot_is_busy_ = false;

    void publish_state(const std::string & state)
    {
        auto message = std_msgs::msg::String();
        message.data = state;

        state_publisher_->publish(message);

        RCLCPP_INFO(
            this->get_logger(),
            "Robot state: %s",
            state.c_str());
    }

    void process_next_task()
    {
        if (robot_is_busy_)
            return;

        if (!food_queue_.empty()) {
            std::string food = food_queue_.front();
            food_queue_.pop_front();

            robot_is_busy_ = true;
            publish_state("BUSY");

            RCLCPP_INFO(
                this->get_logger(),
                "Delivering food: %s",
                food.c_str());

            start_task_timer(true, food);
            return;
        }

        if (!order_queue_.empty()) {
            std::string order = order_queue_.front();
            order_queue_.pop_front();

            robot_is_busy_ = true;
            publish_state("BUSY");

            RCLCPP_INFO(
                this->get_logger(),
                "Helping customer choose food: %s",
                order.c_str());

            start_task_timer(false, order);
        }
    }

    void start_task_timer(
        bool is_delivery,
        const std::string & task_message)
    {
        if (task_timer_)
            task_timer_->cancel();

        task_timer_ = this->create_wall_timer(
            2s,
            [this, is_delivery, task_message]()
            {
                task_timer_->cancel();

                if (is_delivery) {
                    RCLCPP_INFO(
                        this->get_logger(),
                        "Food delivered successfully: %s",
                        task_message.c_str());
                } else {
                    auto message = std_msgs::msg::String();
                    message.data = task_message;

                    kitchen_order_publisher_->publish(message);

                    RCLCPP_INFO(
                        this->get_logger(),
                        "Order sent to kitchen: %s",
                        task_message.c_str());
                }

                robot_is_busy_ = false;
                publish_state("FREE");

                process_next_task();
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

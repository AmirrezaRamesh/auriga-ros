#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <chrono>
#include <map>
#include <memory>
#include <string>

using namespace std::chrono_literals;

class kitchen_node : public rclcpp::Node{
    public:
        kitchen_node() : Node("kitchen_node")
    {
        stock_["Pizza"] = 2;
        stock_["Burger"] = 3;
        stock_["Pasta"] = 3;

        subscription_ = this->create_subscription<std_msgs::msg::String>("order", 10, [this](std_msgs::msg::String::SharedPtr message){
                process_order(message->data);
            }
        );

        RCLCPP_INFO(this->get_logger(), "Kitchen node started!");
        show_stock();
    }

private:

    std::map<std::string, int> stock_;

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;

    std::vector<rclcpp::TimerBase::SharedPtr> preparation_timers_;

    void process_order(const std::string & order)
    {
        std::string food;

        if (order.find("Pizza") != std::string::npos)
            food = "Pizza";
        else if (order.find("Burger") != std::string::npos)
            food = "Burger";
        else if (order.find("Pasta") != std::string::npos)
            food = "Pasta";
        else {
            RCLCPP_WARN(this->get_logger(), "Unknown food in order: %s", order.c_str());
            return;
        }

        if (stock_[food] == 0) {
            RCLCPP_WARN(this->get_logger(), "%s is out of stock!", food.c_str());

            if (stock_["Burger"] > 0)
                food = "Burger";
            else if (stock_["Pasta"] > 0)
                food = "Pasta";
            else if (stock_["Pizza"] > 0)
                food = "Pizza";
            else {
                RCLCPP_ERROR(this->get_logger(), "All food is sold out. Restaurant is closing!");
                return;
            }

            RCLCPP_INFO(this->get_logger(), "Substituting with %s", food.c_str());
        }

        stock_[food]--;

        int preparation_time = 0;

        if (food == "Pizza")
            preparation_time = 15;
        else if (food == "Burger")
            preparation_time = 5;
        else if (food == "Pasta")
            preparation_time = 7;

        RCLCPP_INFO(this->get_logger(), "Preparing %s (%d seconds)", food.c_str(), preparation_time);
        show_stock();

        auto timer = this->create_wall_timer(
            std::chrono::seconds(preparation_time),
            [this, food]() {
                RCLCPP_INFO(this->get_logger(), "%s is READY for delivery!", food.c_str());
            }
        );

        preparation_timers_.push_back(timer);
    }

    void show_stock()
    {
        RCLCPP_INFO(this->get_logger(), "Stock: Pizza=%d, Burger=%d, Pasta=%d",
                    stock_["Pizza"], stock_["Burger"], stock_["Pasta"]);
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<kitchen_node>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "std_srvs/srv/set_bool.hpp"

#include <chrono>
#include <deque>
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

        food_publisher_ = this->create_publisher<std_msgs::msg::String>("food_ready", 10);
        order_subscription_ =
        this->create_subscription<std_msgs::msg::String>("kitchen_order", 10,[this](std_msgs::msg::String::SharedPtr message){
            process_order(message->data);
        }
    );


        state_subscription_ = this->create_subscription<std_msgs::msg::String>("robot_state", 10, [this](std_msgs::msg::String::SharedPtr message){
                robot_is_free_ = (message->data == "FREE");

                RCLCPP_INFO(this->get_logger(), "Robot state received: %s", message->data.c_str());
                if (robot_is_free_)
                    dispatch_food();
            }
        );

        food_available_service_ = this->create_service<std_srvs::srv::Trigger>("food_available",
            [this](
                const std::shared_ptr<std_srvs::srv::Trigger::Request>,
                std::shared_ptr<std_srvs::srv::Trigger::Response> response)
            {
                int total_stock = stock_["Pizza"] + stock_["Burger"] + stock_["Pasta"];
                response->success = restaurant_open_ && total_stock > 0;
                response->message = response->success
                    ? "Food is available"
                    : "No food available or restaurant is closed";
            }
        );
        close_restaurant_service_ = this->create_service<std_srvs::srv::SetBool>("close_restaurant",
            [this](
                const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
                std::shared_ptr<std_srvs::srv::SetBool::Response> response)
            {
                if (request->data) {
                    close_restaurant();
                    response->success = true;
                    response->message = "Restaurant closed";
                } else {
                    response->success = false;
                    response->message = "Send true to close the restaurant";
                }
            }
        );

        RCLCPP_INFO(this->get_logger(), "Kitchen node started!");
        show_stock();
    }

private:

    std::map<std::string, int> stock_;
    std::deque<std::string> ready_food_;

    bool robot_is_free_ = false;
    bool restaurant_open_ = true;
    int next_timer_id_ = 0;

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr food_publisher_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr order_subscription_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr state_subscription_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr food_available_service_;
    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr close_restaurant_service_;

    std::map<int, rclcpp::TimerBase::SharedPtr> preparation_timers_;

    void process_order(const std::string & order)
    {
        if (!restaurant_open_) {
            RCLCPP_WARN(this->get_logger(), "Restaurant is closed. Order rejected.");
            return;
        }

        std::string food;

        if (order.find("Pizza") != std::string::npos)
            food = "Pizza";
        else if (order.find("Burger") != std::string::npos)
            food = "Burger";
        else if (order.find("Pasta") != std::string::npos)
            food = "Pasta";
        else {
            RCLCPP_WARN(this->get_logger(), "Unknown food: %s", order.c_str());
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
                close_restaurant();
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
        int timer_id = next_timer_id_++;

        preparation_timers_[timer_id] = this->create_wall_timer(
            std::chrono::seconds(preparation_time),
            [this, food, order, timer_id]() {
                preparation_timers_[timer_id]->cancel();
                RCLCPP_INFO(this->get_logger(), "%s is ready!", food.c_str());
                ready_food_.push_back("Food: " + food + " | Order: " + order);
                RCLCPP_INFO(this->get_logger(), "Food added to delivery queue. Queue size: %zu", ready_food_.size());
                dispatch_food();
            }
        );

        if (stock_["Pizza"] == 0 &&
            stock_["Burger"] == 0 &&
            stock_["Pasta"] == 0) {
            close_restaurant();
        }
    }

    void dispatch_food()
    {
        if (!robot_is_free_) {
            RCLCPP_INFO(this->get_logger(), "Robot is busy. Prepared food will wait.");
            return;
        }

        if (ready_food_.empty())
            return;

        auto message = std_msgs::msg::String();
        message.data = ready_food_.front();
        robot_is_free_ = false;
        food_publisher_->publish(message);
        RCLCPP_INFO(this->get_logger(), "Sending food to robot: %s", message.data.c_str());
        ready_food_.pop_front();
    }

    void show_stock()
    {
        RCLCPP_INFO(this->get_logger(), "Stock: Pizza=%d, Burger=%d, Pasta=%d",
                    stock_["Pizza"], stock_["Burger"], stock_["Pasta"]);
    }

    void close_restaurant()
    {
        if (!restaurant_open_)
            return;
        restaurant_open_ = false;
        RCLCPP_WARN(this->get_logger(), "Restaurant is closed to new orders.");
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

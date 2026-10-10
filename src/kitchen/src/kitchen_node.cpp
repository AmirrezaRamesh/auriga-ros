#include "rclcpp/rclcpp.hpp"
#include "interface/msg/robot.hpp"
#include "interface/srv/order.hpp"
#include "interface/srv/food_ready.hpp"

#include <memory>
#include <string>
#include <deque>

#include "../include/kitchen/Logger.hpp"
#include "../include/kitchen/Menu.hpp"

class KithcenHandler
{
public:
    KithcenHandler(int period_ms)
        : period_second_(period_ms / 1000.0)
    {
    }

    struct OrderInfo
    {
    public:
        OrderInfo() = default;
        OrderInfo(const Food &food_)
        {
            food = food_;
            remaining_time = food_.preparationTime;
            is_ready = false;
        }
        Food food;
        bool is_ready;
        float remaining_time;
    };

    void run()
    {
        has_ready_food = false;

        for (auto &o : orders)
        {
            if (o.remaining_time <= 0)
            {
                o.is_ready = true;
            }
            o.remaining_time -= period_second_;
        }

        if (robot_state_ == "free")
        {
            for (size_t i = 0; i < orders.size(); i++)
            {
                if (orders.at(i).is_ready)
                {
                    has_ready_food = true;
                    ready_food_info = orders.at(i);
                    orders.erase(orders.begin() + i);
                    break;
                }
            }
        }
    }
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
                LOG_INFO("kitchen", "%s added to que !\n%f remains !", m.name.c_str(), m.remaining);
                return true; // response was successful
            }
        }
        LOG_INFO("kitchen", "there was a no successful order !!!");
        return false;
    }

    OrderInfo get_ready_food_info()
    {
        return ready_food_info;
    }

    bool get_has_food_ready()
    {
        return has_ready_food;
    }

private:
    std::string robot_state_;

    std::deque<OrderInfo> orders = {};
    float period_second_;

    bool has_ready_food = false;
    OrderInfo ready_food_info;
};

using std::placeholders::_1;
using std::placeholders::_2;
using Robot = interface::msg::Robot;
using Order = interface::srv::Order;
using FoodReady = interface::srv::FoodReady;

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

        // timer
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(timer_period_ms),
            std::bind(&KitchenNode::timer_callback, this));

        food_ready_client_ = this->create_client<FoodReady>(
            "food_ready");
    }

private:
    void timer_callback()
    {
        kithcenHandler.run();

        if (kithcenHandler.get_has_food_ready())
        {
            send_request(kithcenHandler.get_ready_food_info().food.name);
        }
    }

    void send_request(std::string name)
    {
        if (!food_ready_client_->service_is_ready())
        {
            RCLCPP_WARN(this->get_logger(), "Waiting for food service...");
            return;
        }

        auto request = std::make_shared<FoodReady::Request>();
        request->name = name;
        std::string sent_name = request->name;

        food_ready_client_->async_send_request(
            request);
    }

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
    rclcpp::Client<FoodReady>::SharedPtr food_ready_client_;
    rclcpp::TimerBase::SharedPtr timer_;
    int timer_period_ms = 200;

    // main controller for kitchen
    KithcenHandler kithcenHandler{timer_period_ms};
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<KitchenNode>());
    rclcpp::shutdown();
    return 0;
}
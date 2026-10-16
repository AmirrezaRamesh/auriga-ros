#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "waiter_robot_interfaces/msg/order.hpp"
#include "waiter_robot_interfaces/srv/food_available.hpp"
#include "waiter_robot_interfaces/srv/food_ready.hpp"
#include "waiter_robot_interfaces/srv/close_restaurant.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

using Order = waiter_robot_interfaces::msg::Order;
using FoodAvailable = waiter_robot_interfaces::srv::FoodAvailable;
using FoodReady = waiter_robot_interfaces::srv::FoodReady;
using CloseRestaurant = waiter_robot_interfaces::srv::CloseRestaurant;

class KitchenNode:public rclcpp::Node{
    using Clock = std::chrono::steady_clock;
    using Key = std::pair<uint64_t , int32_t>;

    struct Reservation{
        std::string requested;
        std::string food;
        int32_t quantity;
    };

    struct CookingOrder{
        Order order;
        rclcpp::Time ready_at;
        bool ready = false;
        bool sending = false;
        bool handed_over = false;
    };

    std::map<std::string , int32_t> stock = {{"Pizza" , 2} , {"Burger" , 3} , {"Pasta" , 3}};
    std::array<std::string , 3> menu = {"Pizza" , "Burger" , "Pasta"};
    std::map<Key , Reservation> reserved;
    std::map<Key , CookingOrder> cooking;

    bool robot_free = false;
    bool closed = false;
    bool closing = false;
    bool close_ack = false;
    bool sending_food = false;

    int64_t close_request_id = -1;
    int64_t ready_request_id = -1;
    Key pending_delivery = {0 , 0};
    Clock::time_point close_deadline = Clock::now();
    Clock::time_point ready_deadline = Clock::now();
    Clock::time_point next_close_notice = Clock::now();
    Clock::time_point next_food_retry = Clock::now();

    rclcpp::Service<FoodAvailable>::SharedPtr availability_srv;
    rclcpp::Subscription<Order>::SharedPtr order_sub;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr state_sub;
    rclcpp::Client<FoodReady>::SharedPtr ready_client;
    rclcpp::Client<CloseRestaurant>::SharedPtr close_client;
    rclcpp::TimerBase::SharedPtr timer;

    int32_t remaining_stock() const{
        int32_t total = 0;
        for(const auto &item:stock){
            total += item.second;
        }
        return total;
    }

    int cooking_time(const std::string &food) const{
        if(food == "Pizza"){
            return 15;
        }
        if(food == "Burger"){
            return 5;
        }
        if(food == "Pasta"){
            return 7;
        }
        return 0;
    }

    void reserve_food(const std::shared_ptr<FoodAvailable::Request> request , std::shared_ptr<FoodAvailable::Response> response){
        response->accepted = false;
        response->assigned_food = "";
        if(request->session_id == 0 || request->order_id <= 0 || request->quantity <= 0){
            return;
        }

        Key key = {request->session_id , request->order_id};
        auto old = reserved.find(key);
        if(old != reserved.end()){
            if(old->second.quantity == request->quantity && old->second.requested == request->requested_food){
                response->accepted = true;
                response->assigned_food = old->second.food;
            }
            return;
        }

        if(closed || stock.find(request->requested_food) == stock.end()){
            return;
        }

        std::string selected;
        if(stock.at(request->requested_food) >= request->quantity){
            selected = request->requested_food;
        }
        else{
            for(const auto &food:menu){
                if(stock.at(food) >= request->quantity){
                    selected = food;
                    break;
                }
            }
        }

        if(selected.empty()){
            RCLCPP_WARN(get_logger() , "No food available for Order %d" , request->order_id);
            return;
        }

        stock.at(selected) -= request->quantity;
        reserved.emplace(key , Reservation{request->requested_food , selected , request->quantity});
        response->accepted = true;
        response->assigned_food = selected;
        RCLCPP_INFO(get_logger() , "Order %d reserved: %d %s" , request->order_id , request->quantity , selected.c_str());
        RCLCPP_INFO(get_logger() , "Remaining stock: %d" , remaining_stock());

        if(remaining_stock() == 0 && !closed){
            closed = true;
            RCLCPP_WARN(get_logger() , "RESTAURANT IS CLOSED! ALL FOOD SOLD OUT!");
        }
    }

    void receive_order(const Order::SharedPtr msg){
        Key key = {msg->session_id , msg->order_id};
        auto item = reserved.find(key);
        if(item == reserved.end()){
            RCLCPP_WARN(get_logger() , "Order %d has no reservation" , msg->order_id);
            return;
        }
        if(item->second.food != msg->food || item->second.quantity != msg->quantity || msg->table_id < 1 || msg->table_id > 3){
            RCLCPP_WARN(get_logger() , "Order %d is invalid" , msg->order_id);
            return;
        }
        if(cooking.count(key)){
            return;
        }

        int seconds = cooking_time(msg->food);
        if(seconds == 0){
            return;
        }
        cooking.emplace(key , CookingOrder{*msg , now() + rclcpp::Duration::from_seconds(seconds) , false , false , false});
        RCLCPP_INFO(get_logger() , "Cooking Order %d: %s (%d seconds)" , msg->order_id , msg->food.c_str() , seconds);
    }

    void state_callback(const std_msgs::msg::Bool::SharedPtr msg){
        robot_free = msg->data;
    }

    void close_restaurant(){
        if(!closed || closing || Clock::now() < next_close_notice || !close_client->service_is_ready()){
            return;
        }

        closing = true;
        next_close_notice = Clock::now() + 2s;
        auto request = std::make_shared<CloseRestaurant::Request>();
        request->reason = "All food is sold out";

        auto callback = [this](rclcpp::Client<CloseRestaurant>::SharedFuture future){
            closing = false;
            close_request_id = -1;
            if(future.get()->success && !close_ack){
                close_ack = true;
                RCLCPP_INFO(get_logger() , "Customer received closing message");
            }
        };

        auto result = close_client->async_send_request(request , callback);
        close_request_id = result.request_id;
        close_deadline = Clock::now() + 1500ms;
    }

    void kitchen_tick(){
        auto current = Clock::now();
        if(closing && current >= close_deadline){
            close_client->remove_pending_request(close_request_id);
            close_request_id = -1;
            closing = false;
            RCLCPP_WARN(get_logger() , "CloseRestaurant timeout, will retry");
        }
        close_restaurant();

        for(auto &item:cooking){
            auto &order = item.second;
            if(!order.ready && now() >= order.ready_at){
                order.ready = true;
                RCLCPP_INFO(get_logger() , "Order %d is READY" , item.first.second);
            }
        }

        if(sending_food && current >= ready_deadline){
            ready_client->remove_pending_request(ready_request_id);
            ready_request_id = -1;
            sending_food = false;
            auto item = cooking.find(pending_delivery);
            if(item != cooking.end()){
                item->second.sending = false;
            }
            next_food_retry = current + 500ms;
            RCLCPP_WARN(get_logger() , "FoodReady timeout, will retry same Order ID");
        }

        if(!robot_free || sending_food || current < next_food_retry || !ready_client->service_is_ready()){
            return;
        }

        for(auto &item:cooking){
            auto &order = item.second;
            if(!order.ready || order.sending || order.handed_over){
                continue;
            }

            auto request = std::make_shared<FoodReady::Request>();
            request->session_id = order.order.session_id;
            request->order_id = order.order.order_id;
            request->table_id = order.order.table_id;
            request->food = order.order.food;
            request->quantity = order.order.quantity;

            Key key = item.first;
            pending_delivery = key;
            order.sending = true;
            sending_food = true;

            auto callback = [this , key](rclcpp::Client<FoodReady>::SharedFuture future){
                if(!sending_food || pending_delivery != key){
                    return;
                }
                sending_food = false;
                ready_request_id = -1;
                auto it = cooking.find(key);
                if(it == cooking.end()){
                    return;
                }
                it->second.sending = false;
                if(future.get()->accepted){
                    it->second.handed_over = true;
                    RCLCPP_INFO(get_logger() , "Order %d handed to Robot" , key.second);
                }
                else{
                    next_food_retry = Clock::now() + 500ms;
                }
            };

            auto result = ready_client->async_send_request(request , callback);
            ready_request_id = result.request_id;
            ready_deadline = Clock::now() + 1500ms;
            break;
        }
    }

public:
    KitchenNode():Node("kitchen_node"){
        availability_srv = create_service<FoodAvailable>("/food_available" , std::bind(&KitchenNode::reserve_food , this , _1 , _2));
        order_sub = create_subscription<Order>("/order" , 10 , std::bind(&KitchenNode::receive_order , this , _1));
        auto qos = rclcpp::QoS(1).reliable().transient_local();
        state_sub = create_subscription<std_msgs::msg::Bool>("/robot_state" , qos , std::bind(&KitchenNode::state_callback , this , _1));
        ready_client = create_client<FoodReady>("/food_ready");
        close_client = create_client<CloseRestaurant>("/close_resturant");
        timer = create_wall_timer(200ms , std::bind(&KitchenNode::kitchen_tick , this));
        RCLCPP_INFO(get_logger() , "Kitchen started!");
    }
};

int main(int argc , char **argv){
    rclcpp::init(argc , argv);
    rclcpp::spin(std::make_shared<KitchenNode>());
    rclcpp::shutdown();
    return 0;
}
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <set>
#include <utility>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "waiter_robot_interfaces/msg/order.hpp"
#include "waiter_robot_interfaces/srv/order_phase.hpp"
#include "waiter_robot_interfaces/srv/food_available.hpp"
#include "waiter_robot_interfaces/srv/food_ready.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

using Order = waiter_robot_interfaces::msg::Order;
using OrderPhase = waiter_robot_interfaces::srv::OrderPhase;
using FoodAvailable = waiter_robot_interfaces::srv::FoodAvailable;
using FoodReady = waiter_robot_interfaces::srv::FoodReady;

class RobotNode:public rclcpp::Node{
    using Clock = std::chrono::steady_clock;
    using Key = std::pair<uint64_t , int32_t>;

    bool synced = false;
    bool taking = false;
    bool checking = false;
    bool delivering = false;
    bool food_pending = false;

    uint64_t session = 0;
    uint32_t active_cycle = 0;
    uint32_t finished_cycle = 0;
    uint32_t min_cycle = 1;
    int64_t food_request_id = -1;

    Clock::time_point taking_deadline = Clock::now();
    Clock::time_point last_heartbeat = Clock::now();
    Clock::time_point food_deadline = Clock::now();
    Clock::time_point food_retry = Clock::now();

    Order checking_order;
    Order current_delivery;
    std::queue<Order> orders;
    std::set<Key> received_ids;
    std::set<Key> delivered_ids;

    rclcpp::Publisher<Order>::SharedPtr order_pub;
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr state_pub;
    rclcpp::Subscription<Order>::SharedPtr customer_sub;
    rclcpp::Service<OrderPhase>::SharedPtr phase_srv;
    rclcpp::Service<FoodReady>::SharedPtr ready_srv;
    rclcpp::Client<FoodAvailable>::SharedPtr food_client;
    rclcpp::TimerBase::SharedPtr timer;
    rclcpp::TimerBase::SharedPtr delivery_timer;

    bool is_free() const{
        return synced && !taking && !checking && !delivering;
    }

    void publish_state(){
        std_msgs::msg::Bool message;
        message.data = is_free();
        state_pub->publish(message);
    }

    void phase_callback(const std::shared_ptr<OrderPhase::Request> request , std::shared_ptr<OrderPhase::Response> response){
        response->accepted = false;
        response->message = "Rejected";
        uint32_t id = request->cycle_id;

        if(request->session_id == 0){
            return;
        }

        if(request->phase == OrderPhase::Request::HEARTBEAT){
            if(synced && request->session_id == session){
                last_heartbeat = Clock::now();
                response->accepted = true;
                response->message = "HEARTBEAT ACK";
            }
            return;
        }

        if(request->phase == OrderPhase::Request::RESYNC){
            if(taking || checking || delivering || !orders.empty()){
                response->message = "Robot busy";
                return;
            }

            if(request->session_id != session){
                session = request->session_id;
                finished_cycle = 0;
                min_cycle = request->cycle_id;
            }
            else{
                min_cycle = std::max(min_cycle , request->cycle_id);
            }
            synced = true;
            last_heartbeat = Clock::now();
            active_cycle = 0;
            response->accepted = true;
            response->message = "RESYNC ACK";
            publish_state();
            RCLCPP_INFO(get_logger() , "Robot synchronized with Customer");
            return;
        }

        if(!synced || request->session_id != session || id == 0 || id < min_cycle){
            response->message = "RESYNC required";
            return;
        }

        if(request->phase == OrderPhase::Request::START){
            if(taking && id == active_cycle){
                response->accepted = true;
                response->message = "START ACK";
                return;
            }

            if(id <= finished_cycle || !is_free() || !orders.empty() || !food_client->service_is_ready()){
                return;
            }

            taking = true;
            active_cycle = id;
            taking_deadline = Clock::now() + 4s;
            publish_state();
            response->accepted = true;
            response->message = "START ACK";
            RCLCPP_INFO(get_logger() , "Cycle %u: BUSY" , id);
        }
        else if(request->phase == OrderPhase::Request::FINISH){
            if(!taking && id == finished_cycle){
                response->accepted = true;
                response->message = "FINISH ACK";
                return;
            }

            if(!taking || id != active_cycle){
                return;
            }

            taking = false;
            finished_cycle = id;
            active_cycle = 0;
            publish_state();
            response->accepted = true;
            response->message = "FINISH ACK";
            RCLCPP_INFO(get_logger() , "Cycle %u: FINISHED" , id);
        }
    }

    void customer_callback(const Order::SharedPtr message){
        if(!synced || message->session_id != session || message->order_id <= 0 || static_cast<uint32_t>(message->order_id) > finished_cycle){
            return;
        }

        Key key = {message->session_id , message->order_id};
        if(!received_ids.insert(key).second){
            return;
        }

        orders.push(*message);
        RCLCPP_INFO(get_logger() , "Received Order %d from Table %d" , message->order_id , message->table_id);
    }

    void check_food(){
        if(!food_client->service_is_ready()){
            return;
        }

        auto request = std::make_shared<FoodAvailable::Request>();
        request->session_id = checking_order.session_id;
        request->order_id = checking_order.order_id;
        request->requested_food = checking_order.food;
        request->quantity = checking_order.quantity;

        food_pending = true;
        Key key = {checking_order.session_id , checking_order.order_id};

        auto callback = [this , key](rclcpp::Client<FoodAvailable>::SharedFuture future){
            if(!checking || !food_pending || Key{checking_order.session_id , checking_order.order_id} != key){
                return;
            }

            food_pending = false;
            food_request_id = -1;
            auto response = future.get();

            if(response->accepted){
                Order confirmed = checking_order;
                confirmed.food = response->assigned_food;
                order_pub->publish(confirmed);
                RCLCPP_INFO(get_logger() , "Order %d accepted: %s" , confirmed.order_id , confirmed.food.c_str());
            }
            else{
                RCLCPP_WARN(get_logger() , "Order %d rejected by Kitchen" , checking_order.order_id);
            }

            checking = false;
            publish_state();
        };

        auto future = food_client->async_send_request(request , callback);
        food_request_id = future.request_id;
        food_deadline = Clock::now() + 1500ms;
    }

    void timer_callback(){
        auto current = Clock::now();

        if(synced && current - last_heartbeat >= 3s){
            finished_cycle = std::max(finished_cycle , active_cycle);
            taking = false;
            active_cycle = 0;
            synced = false;
            RCLCPP_ERROR(get_logger() , "Customer heartbeat lost. Robot NOT_READY");
        }

        if(taking && current >= taking_deadline){
            finished_cycle = std::max(finished_cycle , active_cycle);
            taking = false;
            active_cycle = 0;
            synced = false;
            RCLCPP_ERROR(get_logger() , "TAKING_ORDER timed out. Robot NOT_READY until RESYNC");
        }

        if(checking){
            if(food_pending && current >= food_deadline){
                food_client->remove_pending_request(food_request_id);
                food_request_id = -1;
                food_pending = false;
                food_retry = current + 500ms;
                RCLCPP_WARN(get_logger() , "FoodAvailable timeout. Retrying the same Order ID");
            }
            if(!food_pending && current >= food_retry){
                check_food();
            }
        }
        else if(is_free() && !orders.empty() && food_client->service_is_ready()){
            checking_order = orders.front();
            orders.pop();
            checking = true;
            food_retry = current;
            check_food();
        }

        publish_state();
    }

    void food_ready_callback(const std::shared_ptr<FoodReady::Request> request , std::shared_ptr<FoodReady::Response> response){
        response->accepted = false;

        if(request->session_id == 0 || request->order_id <= 0 || request->quantity <= 0){
            return;
        }

        Key key = {request->session_id , request->order_id};
        if(delivered_ids.count(key)){
            response->accepted = true;
            return;
        }

        if(!is_free()){
            return;
        }

        delivering = true;
        delivered_ids.insert(key);
        current_delivery.session_id = request->session_id;
        current_delivery.order_id = request->order_id;
        current_delivery.table_id = request->table_id;
        current_delivery.food = request->food;
        current_delivery.quantity = request->quantity;
        response->accepted = true;
        publish_state();

        RCLCPP_INFO(get_logger() , "Delivering Order %d" , request->order_id);
        delivery_timer = create_wall_timer(1s , std::bind(&RobotNode::delivery_callback , this));
    }

    void delivery_callback(){
        delivery_timer->cancel();
        RCLCPP_INFO(get_logger() , "Order %d delivered to Table %d" , current_delivery.order_id , current_delivery.table_id);
        delivering = false;
        publish_state();
    }

public:
    RobotNode():Node("robot_node"){
        order_pub = create_publisher<Order>("/order" , 10);
        auto qos = rclcpp::QoS(1).reliable().transient_local();
        state_pub = create_publisher<std_msgs::msg::Bool>("/robot_state" , qos);
        customer_sub = create_subscription<Order>("/customer_order" , 10 , std::bind(&RobotNode::customer_callback , this , _1));
        phase_srv = create_service<OrderPhase>("/order_phase" , std::bind(&RobotNode::phase_callback , this , _1 , _2));
        ready_srv = create_service<FoodReady>("/food_ready" , std::bind(&RobotNode::food_ready_callback , this , _1 , _2));
        food_client = create_client<FoodAvailable>("/food_available");
        timer = create_wall_timer(200ms , std::bind(&RobotNode::timer_callback , this));
        publish_state();
        RCLCPP_INFO(get_logger() , "Robot started in NOT_READY mode");
    }
};

int main(int argc , char **argv){
    rclcpp::init(argc , argv);
    rclcpp::spin(std::make_shared<RobotNode>());
    rclcpp::shutdown();
    return 0;
}
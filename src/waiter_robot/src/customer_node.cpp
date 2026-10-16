#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <random>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "waiter_robot_interfaces/msg/order.hpp"
#include "waiter_robot_interfaces/srv/order_phase.hpp"
#include "waiter_robot_interfaces/srv/close_restaurant.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

using Order = waiter_robot_interfaces::msg::Order;
using OrderPhase = waiter_robot_interfaces::srv::OrderPhase;
using CloseRestaurant = waiter_robot_interfaces::srv::CloseRestaurant;

class CustomerNode:public rclcpp::Node{
    using Clock = std::chrono::steady_clock;
    enum class State{WAIT , SYNCING , FREE , STARTING , BUSY , FINISHING , ORDER , CLOSED};

    State state = State::WAIT;
    Clock::time_point deadline = Clock::now();
    Clock::time_point request_deadline = Clock::now();
    uint64_t session = 0;
    uint32_t cycle = 1;
    uint8_t pending_phase = 0;
    int64_t request_id = -1;
    int64_t heartbeat_id = -1;
    bool connected = false;
    Clock::time_point heartbeat_deadline = Clock::now();
    Clock::time_point next_heartbeat = Clock::now();
    int attempts = 0;
    bool closed = false;

    std::array<std::string , 3> menu = {"Pizza" , "Burger" , "Pasta"};
    rclcpp::Publisher<Order>::SharedPtr order_pub;
    rclcpp::Client<OrderPhase>::SharedPtr phase_client;
    rclcpp::Client<OrderPhase>::SharedPtr heartbeat_client;
    rclcpp::Service<CloseRestaurant>::SharedPtr close_srv;
    rclcpp::TimerBase::SharedPtr timer;

    void send_phase(uint8_t phase){
        if(!phase_client->service_is_ready()){
            return;
        }

        auto request = std::make_shared<OrderPhase::Request>();
        request->session_id = session;
        request->cycle_id = cycle;
        request->phase = phase;

        pending_phase = phase;
        attempts++;
        state = phase == OrderPhase::Request::RESYNC ? State::SYNCING : (phase == OrderPhase::Request::START ? State::STARTING : State::FINISHING);

        auto callback = [this , phase , id = cycle](rclcpp::Client<OrderPhase>::SharedFuture future){
            if(id != cycle || phase != pending_phase){
                return;
            }

            State expected = phase == OrderPhase::Request::RESYNC ? State::SYNCING : (phase == OrderPhase::Request::START ? State::STARTING : State::FINISHING);
            if(state != expected){
                return;
            }

            request_id = -1;
            auto response = future.get();
            auto current = Clock::now();

            if(!response->accepted){
                if(attempts >= 3){
                    recover();
                }
                else{
                    deadline = current + 300ms;
                }
                return;
            }

            attempts = 0;

            if(phase == OrderPhase::Request::RESYNC){
                connected = true;
                next_heartbeat = current;
                if(closed){
                    state = State::CLOSED;
                    return;
                }
                state = State::FREE;
                deadline = current + 2s;
                RCLCPP_INFO(get_logger() , "Cycle %u: FREE" , cycle);
            }
            else if(phase == OrderPhase::Request::START){
                state = State::BUSY;
                deadline = current + 2s;
                RCLCPP_INFO(get_logger() , "Cycle %u: START ACK" , cycle);
                if(closed){
                    send_phase(OrderPhase::Request::FINISH);
                }
            }
            else{
                RCLCPP_INFO(get_logger() , "Cycle %u: FINISH ACK" , cycle);
                if(closed){
                    state = State::CLOSED;
                    return;
                }
                publish_order();
                state = State::ORDER;
                deadline = current + 1s;
            }
        };

        auto future = phase_client->async_send_request(request , callback);
        request_id = future.request_id;
        request_deadline = Clock::now() + 1s;
    }

    void recover(){
        connected = false;
        if(heartbeat_id >= 0){
            heartbeat_client->remove_pending_request(heartbeat_id);
            heartbeat_id = -1;
        }
        if(request_id >= 0){
            phase_client->remove_pending_request(request_id);
            request_id = -1;
        }
        if(pending_phase != OrderPhase::Request::RESYNC){
            cycle++;
        }
        attempts = 0;
        state = State::WAIT;
        deadline = Clock::now() + 500ms;
        RCLCPP_WARN(get_logger() , "Handshake timeout. Resynchronizing...");
    }

    void send_heartbeat(){
        if(!connected){
            return;
        }

        auto current = Clock::now();
        if(heartbeat_id >= 0 && current >= heartbeat_deadline){
            heartbeat_client->remove_pending_request(heartbeat_id);
            heartbeat_id = -1;
        }
        if(heartbeat_id >= 0 || current < next_heartbeat || !heartbeat_client->service_is_ready()){
            return;
        }

        auto request = std::make_shared<OrderPhase::Request>();
        request->session_id = session;
        request->cycle_id = cycle;
        request->phase = OrderPhase::Request::HEARTBEAT;
        auto callback = [this](rclcpp::Client<OrderPhase>::SharedFuture future){
            heartbeat_id = -1;
            if(!future.get()->accepted && connected){
                RCLCPP_WARN(get_logger() , "Heartbeat rejected. Resynchronizing...");
                recover();
            }
        };
        auto result = heartbeat_client->async_send_request(request , callback);
        heartbeat_id = result.request_id;
        heartbeat_deadline = current + 1200ms;
        next_heartbeat = current + 500ms;
    }

    void tick(){
        auto current = Clock::now();
        send_heartbeat();

        if(state == State::CLOSED){
            return;
        }

        if(state == State::WAIT){
            if(current >= deadline && phase_client->service_is_ready() && order_pub->get_subscription_count() > 0){
                send_phase(OrderPhase::Request::RESYNC);
            }
            return;
        }

        if(state == State::SYNCING || state == State::STARTING || state == State::FINISHING){
            if(request_id >= 0 && current >= request_deadline){
                phase_client->remove_pending_request(request_id);
                request_id = -1;
                RCLCPP_WARN(get_logger() , "Service response timeout, cycle %u" , cycle);
                if(attempts >= 3){
                    recover();
                    return;
                }
                deadline = current + 300ms;
            }
            if(request_id < 0 && current >= deadline){
                send_phase(pending_phase);
            }
            return;
        }

        if(current < deadline){
            return;
        }

        if(state == State::FREE){
            send_phase(OrderPhase::Request::START);
        }
        else if(state == State::BUSY){
            send_phase(OrderPhase::Request::FINISH);
        }
        else if(state == State::ORDER){
            cycle++;
            state = State::FREE;
            deadline = current + 2s;
            RCLCPP_INFO(get_logger() , "Cycle %u: FREE" , cycle);
        }
    }

    void publish_order(){
        Order order;
        order.session_id = session;
        order.order_id = static_cast<int32_t>(cycle);
        order.table_id = ((cycle - 1) % 3) + 1;
        order.food = menu[(cycle - 1) % 3];
        order.quantity = cycle % 3 == 0 ? 2 : 1;
        order_pub->publish(order);
        RCLCPP_INFO(get_logger() , "Order %d: Table %d, %d %s" , order.order_id , order.table_id , order.quantity , order.food.c_str());
    }

    void close_callback(const std::shared_ptr<CloseRestaurant::Request> request , std::shared_ptr<CloseRestaurant::Response> response){
        bool was_closed = closed;
        closed = true;
        response->success = true;
        if(!was_closed){
            RCLCPP_WARN(get_logger() , "RESTAURANT IS CLOSED: %s" , request->reason.c_str());
        }

        if(state == State::BUSY){
            attempts = 0;
            send_phase(OrderPhase::Request::FINISH);
        }
        else if(state != State::STARTING && state != State::FINISHING && state != State::SYNCING){
            state = State::CLOSED;
        }
    }

public:
    CustomerNode():Node("customer_node"){
        std::random_device random;
        session = (static_cast<uint64_t>(random()) << 32) | random();
        if(session == 0){
            session = 1;
        }
        order_pub = create_publisher<Order>("/customer_order" , 10);
        phase_client = create_client<OrderPhase>("/order_phase");
        heartbeat_client = create_client<OrderPhase>("/order_phase");
        close_srv = create_service<CloseRestaurant>("/close_resturant" , std::bind(&CustomerNode::close_callback , this , _1 , _2));
        timer = create_wall_timer(100ms , std::bind(&CustomerNode::tick , this));
        RCLCPP_INFO(get_logger() , "Customer started with session %llu" , static_cast<unsigned long long>(session));
    }
};

int main(int argc , char **argv){
    rclcpp::init(argc , argv);
    rclcpp::spin(std::make_shared<CustomerNode>());
    rclcpp::shutdown();
    return 0;
}
#include <chrono>
#include <functional>
#include <memory>
#include<random>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int64.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

class OddNode : public rclcpp::Node
{
  public:
    OddNode()
    : Node("odd_node")
    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<int64_t> distrib(1 , 1000000);
        number_ = distrib(gen);
        RCLCPP_INFO(this->get_logger(), "The opration start with random number: '%ld'", number_);
        publisher_ = this->create_publisher<std_msgs::msg::Int64>("collatz_number", 10);
        timer_ = this->create_wall_timer(
        100ms , std::bind(&OddNode::timer_callback, this));
        subscription_ = this->create_subscription<std_msgs::msg::Int64>(
        "collatz_number", 10, std::bind(&OddNode::collatz_number_callback, this, _1));  
       
    }

  private:

    void collatz_number_callback(const std_msgs::msg::Int64 & msg)
    {
      number_ = msg.data;  
      RCLCPP_INFO(this->get_logger(), "I heard: '%ld'", msg.data);
    }

    
    void timer_callback()
    {
      auto message = std_msgs::msg::Int64();
      if (number_ > 1 && number_ % 2 != 0){
        number_ = (number_ * 3) + 1;
        message.data = number_;
        RCLCPP_INFO(this->get_logger(), "Odd node Publishing: '%ld'", message.data);
        publisher_->publish(message);
      }else if(number_ > 1 && number_ % 2 == 0) {
       message.data = number_;
       RCLCPP_INFO(this->get_logger(),"Interfacing Even number to Even node: '%ld'" , message.data);
       publisher_->publish(message); 
        
      }else{
         RCLCPP_INFO(this->get_logger(), "Reached 1! Done : '%ld'", number_);
         timer_->cancel();
      }
    }

    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<std_msgs::msg::Int64>::SharedPtr publisher_;
    rclcpp::Subscription<std_msgs::msg::Int64>::SharedPtr subscription_;
    int64_t number_ = 0;
     
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<OddNode>());
  rclcpp::shutdown();
  return 0;
}
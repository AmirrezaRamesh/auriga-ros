#include <chrono>
#include <functional>
#include <memory>


#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int64.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

class EvenNode : public rclcpp::Node
{
  public:
    EvenNode()
    : Node("even_node")
    {
        subscription_ = this->create_subscription<std_msgs::msg::Int64>(
        "collatz_number", 10, std::bind(&EvenNode::collatz_number_callback, this, _1));  
        publisher_ = this->create_publisher<std_msgs::msg::Int64>("collatz_number", 10);
        timer_ = this->create_wall_timer(
        100ms , std::bind(&EvenNode::timer_callback, this));
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
      if (number_ > 1 && number_ % 2 == 0){
        number_ = number_ / 2;
        message.data = number_;
        RCLCPP_INFO(this->get_logger(), "Even node Publishing: '%ld'", message.data);
        publisher_->publish(message);
      }else if(number_ > 1 && number_ % 2 != 0){
         RCLCPP_INFO(this->get_logger(), "The Even number is Odd now  : '%ld'", number_);
      RCLCPP_INFO(this->get_logger(), "Even node Publishing: '%ld'", message.data);
      publisher_->publish(message);
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
  rclcpp::spin(std::make_shared<EvenNode>());
  rclcpp::shutdown();
  return 0;
}
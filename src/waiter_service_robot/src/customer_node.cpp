
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <chrono>
#include <functional>
#include <memory>

using namespace std::chrono_literals;

class customer_node : public rclcpp::Node
{
public:

    
    customer_node() : Node("customer_node")
    {
        publisher_ = this->create_publisher<std_msgs::msg::String>("order", 10);
        timer_ = this->create_wall_timer(5s,std::bind(&customer_node::publish_order, this));
        RCLCPP_INFO(this->get_logger(),"Customer node has started!");}


private:

    void publish_order(){
        auto message = std_msgs::msg::String();
        message.data = "Table 1 requests Pizza";
        publisher_->publish(message);
        RCLCPP_INFO(this->get_logger(),"Published order: %s",message.data.c_str());
    }
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<customer_node>();
    rclcpp::spin(node);
    rclcpp::shutdown();


    return 0;

}

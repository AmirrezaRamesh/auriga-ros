#include <chrono>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "interface/srv/order.hpp"
#include "../../kitchen/include/kitchen/Menu.hpp"
#include "../../kitchen/include/kitchen/Logger.hpp"

using namespace std::chrono_literals;

using Order = interface::srv::Order;

class CustomerNode : public rclcpp::Node
{
public:
  CustomerNode()
      : Node("customer_node")
  {
    order_client_ = this->create_client<Order>(
        "order");

    timer_ = this->create_wall_timer(
        std::chrono::seconds(5),
        std::bind(&CustomerNode::timer_callback, this));
  }

  void send_request(std::string name)
  {
    if (!order_client_->service_is_ready())
    {
      RCLCPP_WARN(this->get_logger(), "Waiting for order service...");
      return;
    }

    auto request = std::make_shared<Order::Request>();
    request->name = name;
    std::string sent_name = request->name;

    order_client_->async_send_request(
        request,
        [this, sent_name](rclcpp::Client<Order>::SharedFuture future)
        {
          auto response = future.get();
          if (!response->success)
          {
            RCLCPP_INFO(this->get_logger(), "oh sorry we can not provide your order please order something else ...!");
          }
        });
  }

  // change order every loop
  std::string get_order()
  {
    auto order = menu.at(order_counter_);
    order_counter_ = (order_counter_ + 1) % menu.size();

    // order was finished !
    while (order.remaining <= 0)
    {
      RCLCPP_WARN(this->get_logger(), "sorry we are out of item %s !", order.name.c_str());
      order = menu.at(order_counter_);
      order_counter_ = (order_counter_ + 1) % menu.size();
    }

    return order.name;
  }

private:
  void timer_callback()
  {
    send_request(get_order());
  }

  rclcpp::Client<Order>::SharedPtr order_client_;
  rclcpp::TimerBase::SharedPtr timer_;

  int order_counter_ = 0;
};

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);

  auto customer_node = std::make_shared<CustomerNode>();

  rclcpp::spin(customer_node);

  rclcpp::shutdown();

  return 0;
}
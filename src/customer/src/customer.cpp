#include <chrono>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "interface/srv/order.hpp"
#include "std_srvs/srv/empty.hpp"

#include "../../kitchen/include/kitchen/Menu.hpp"

using namespace std::chrono_literals;

using Order = interface::srv::Order;
using Empty = std_srvs::srv::Empty;

using std::placeholders::_1;
using std::placeholders::_2;

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

    closing_service_ = this->create_service<Empty>(
        "closing",
        std::bind(&CustomerNode::closing_service_callback, this, _1, _2));
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
            RCLCPP_INFO(this->get_logger(), "kithen can not provide this order !");
          }
        });
  }

  // change order every loop
  std::string get_order()
  {
    auto order = menu.at(order_counter_);
    order_counter_ = (order_counter_ + 1) % menu.size();

    return order.name;
  }

private:
  void timer_callback()
  {
    if (!is_close)
    {
      send_request(get_order());
    }
    else
    {
      RCLCPP_WARN_ONCE(this->get_logger(), "closing the restaurant !!!");
    }
  }

  void closing_service_callback(
      const std::shared_ptr<Empty::Request> request,
      std::shared_ptr<Empty::Response> response)
  {
    (void)request;
    (void)response;
    is_close = true;
  }

  rclcpp::Client<Order>::SharedPtr order_client_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Service<Empty>::SharedPtr closing_service_;

  int order_counter_ = 0;
  bool is_close = false;
};

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);

  auto customer_node = std::make_shared<CustomerNode>();

  rclcpp::spin(customer_node);

  rclcpp::shutdown();

  return 0;
}
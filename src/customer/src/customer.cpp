#include <chrono>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "interface/srv/order.hpp"

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
  }

  void send_request(std::string name)
  {
    // Wait for the server
    while (!order_client_->wait_for_service(std::chrono::seconds(1)))
    {
      if (!rclcpp::ok())
      {
        RCLCPP_ERROR(this->get_logger(), "ROS shutdown");
        return;
      }

      RCLCPP_INFO(this->get_logger(), "Waiting for order service...");
    }

    // Create request
    auto request = std::make_shared<Order::Request>();

    request->name = name;

    // Send request
    auto future = order_client_->async_send_request(request);

    // Wait for response
    if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), future) == rclcpp::FutureReturnCode::SUCCESS)
    {
      auto response = future.get();
    }
    else
    {
      RCLCPP_ERROR(this->get_logger(), "Failed to call service");
    }
  }

private:
  rclcpp::Client<Order>::SharedPtr order_client_;
};

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);

  auto client = std::make_shared<CustomerNode>();

  client->send_request("burger");

  rclcpp::shutdown();

  return 0;
}
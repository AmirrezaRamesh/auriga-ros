// example_sub.cpp
// Subscribes to /map (transient_local so we get the already-published map),
// then copies it into a 2D vector: grid[y][x].
//
// Values: 0 = free, 100 = occupied, -1 = unknown.

#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"

class MapSubscriber : public rclcpp::Node
{
public:
  MapSubscriber() : Node("example_sub")
  {
    // Must match the publisher: reliable + transient_local.
    auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();

    sub_ = create_subscription<nav_msgs::msg::OccupancyGrid>(
      "/map", qos,
      std::bind(&MapSubscriber::mapCallback, this, std::placeholders::_1));
  }

private:
  void mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
  {
    const size_t width = msg->info.width;
    const size_t height = msg->info.height;

    // Sanity check before indexing.
    if (msg->data.size() != width * height) {
      RCLCPP_ERROR(get_logger(), "Map size mismatch: %zu cells for %zux%zu",
                   msg->data.size(), width, height);
      return;
    }

    // Resize to [height][width] and fill (row-major: index = y * width + x).
    grid_.assign(height, std::vector<int8_t>(width, -1));
    for (size_t y = 0; y < height; ++y) {
      for (size_t x = 0; x < width; ++x) {
        grid_[y][x] = msg->data[y * width + x];
      }
    }

    RCLCPP_INFO(get_logger(), "Got map %zux%zu, resolution %.2f m/cell",
                width, height, msg->info.resolution);
    printGrid();
  }

  void printGrid() const
  {
    for (const auto & row : grid_) {
      std::string line;
      for (int8_t cell : row) {
        line += (cell > 0) ? '#' : (cell == 0 ? '.' : '?');
      }
      RCLCPP_INFO(get_logger(), "%s", line.c_str());
    }
  }

  std::vector<std::vector<int8_t>> grid_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MapSubscriber>());
  rclcpp::shutdown();
  return 0;
}

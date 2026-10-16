#include <fstream>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"

class MapLoader : public rclcpp::Node
{
public:
  MapLoader() : Node("map_loader")
  {
    map_file_ = "/home/amir/ROS/src/cpp_parameters/src/map.txt";
    frame_id_   = "map";
    resolution_ = 1.0;

    auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    pub_ = create_publisher<nav_msgs::msg::OccupancyGrid>("/map", qos);

    if (loadAndPublish()) {
      RCLCPP_INFO(get_logger(), "Map published on /map");
    } else {
      RCLCPP_ERROR(get_logger(), "Map was not published");
    }
  }

private:
  bool loadAndPublish()
  {
    std::ifstream file(map_file_);
    if (!file.is_open()) {
      RCLCPP_ERROR(get_logger(), "Cannot open '%s'", map_file_.c_str());
      return false;
    }

    std::vector<std::string> rows;
    std::string line;
    while (std::getline(file, line)) {
      while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
        line.pop_back();
      }
      if (!line.empty()) {
        rows.push_back(line);
      }
    }

    const size_t kSize = 20;
    if (rows.size() != kSize) {
      RCLCPP_ERROR(get_logger(), "Expected %zu rows, got %zu", kSize, rows.size());
      return false;
    }

    nav_msgs::msg::OccupancyGrid msg;
    msg.header.stamp = now();
    msg.header.frame_id = frame_id_;
    msg.info.resolution = static_cast<float>(resolution_);
    msg.info.width = kSize;
    msg.info.height = kSize;
    msg.info.origin.position.x = 0.0;
    msg.info.origin.position.y = 0.0;
    msg.info.origin.orientation.w = 1.0;
    msg.data.reserve(kSize * kSize);

    for (size_t y = 0; y < kSize; ++y) {
      if (rows[y].size() != kSize) {
        RCLCPP_ERROR(get_logger(), "Row %zu has %zu columns, expected %zu",
                     y, rows[y].size(), kSize);
        return false;
      }
      for (size_t x = 0; x < kSize; ++x) {
        char c = rows[y][x];
        if (c == '0') {
          msg.data.push_back(0);      
        } else if (c == '1') {
          msg.data.push_back(100); 
        } else {
          RCLCPP_ERROR(get_logger(), "Bad character '%c' at row %zu, col %zu", c, y, x);
          return false;
        }
      }
    }

    pub_->publish(msg);
    return true;
  }

  std::string map_file_;
  std::string frame_id_;
  double resolution_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MapLoader>());
  rclcpp::shutdown();
  return 0;
}

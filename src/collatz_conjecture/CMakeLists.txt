cmake_minimum_required(VERSION 3.5)
project(collatz_conjecture)


if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(odd_node src/odd_node.cpp)
ament_target_dependencies(odd_node rclcpp std_msgs)

add_executable(even_node src/even_node.cpp)
ament_target_dependencies(even_node rclcpp std_msgs)

install(TARGETS
  odd_node
  even_node
  DESTINATION lib/${PROJECT_NAME})

ament_package()
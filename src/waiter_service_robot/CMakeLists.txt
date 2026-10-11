
cmake_minimum_required(VERSION 3.8)
project(waiter_service_robot)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

add_executable(customer_node src/customer_node.cpp)
ament_target_dependencies(customer_node
  rclcpp
  std_msgs
  std_srvs
)

add_executable(robot_node src/robot_node.cpp)
ament_target_dependencies(robot_node
  rclcpp
  std_msgs
)

add_executable(kitchen_node src/kitchen_node.cpp)
ament_target_dependencies(kitchen_node
  rclcpp
  std_msgs
  std_srvs
)

install(TARGETS
  customer_node
  robot_node
  kitchen_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()

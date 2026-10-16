cmake_minimum_required(VERSION 3.8)
project(waiter_robot)

# Compiler warnings
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(waiter_robot_interfaces REQUIRED)

# Create executables
add_executable(customer_node src/customer_node.cpp)
add_executable(robot_node src/robot_node.cpp)
add_executable(kitchen_node src/kitchen_node.cpp)

# Configure all three nodes
foreach(target customer_node robot_node kitchen_node)

  target_compile_features(${target} PUBLIC cxx_std_17)

  target_include_directories(${target} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )

  ament_target_dependencies(${target}
    rclcpp
    std_msgs
    waiter_robot_interfaces
  )

endforeach()

# Install executables
install(
  TARGETS
    customer_node
    robot_node
    kitchen_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
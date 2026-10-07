cmake_minimum_required(VERSION 3.8)
project(kitchen)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(interface REQUIRED)


add_executable(kitchen
  src/kitchen_node.cpp
  src/Logger.cpp
)
ament_target_dependencies(kitchen
  rclcpp
  interface
)

install(TARGETS
  kitchen
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
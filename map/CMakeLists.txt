cmake_minimum_required(VERSION 3.8)
project(cpp_parameters)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav_msgs REQUIRED)

add_executable(map_loader src/map_loader.cpp)
ament_target_dependencies(map_loader nav_msgs rclcpp)

add_executable(example_sub src/example_sub.cpp)
ament_target_dependencies(example_sub nav_msgs rclcpp)

install(TARGETS
    map_loader
    example_sub
  DESTINATION lib/${PROJECT_NAME}
)

install(FILES
  src/map.txt
  DESTINATION share/${PROJECT_NAME}
)

ament_package()


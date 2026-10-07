cmake_minimum_required(VERSION 3.8)
project(customer)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(interface REQUIRED)


add_executable(customer
  src/customer.cpp
)
ament_target_dependencies(customer
  rclcpp
  interface
)

install(TARGETS
  customer
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
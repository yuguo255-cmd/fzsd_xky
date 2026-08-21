cmake_minimum_required(VERSION 3.8)
project(rm_bt_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(rm_referee_interfaces REQUIRED)
find_package(rm_interfaces REQUIRED)

add_executable(referee_to_serial_bridge
  src/referee_to_serial_bridge.cpp
)
ament_target_dependencies(referee_to_serial_bridge
  rclcpp
  rm_referee_interfaces
  rm_interfaces
)

add_executable(goal_pose_bridge
  src/goal_pose_bridge.cpp
)
ament_target_dependencies(goal_pose_bridge
  rclcpp
  rclcpp_action
  geometry_msgs
  nav2_msgs
)

install(TARGETS
  referee_to_serial_bridge
  goal_pose_bridge
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()

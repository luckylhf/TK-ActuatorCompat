cmake_minimum_required(VERSION 3.8)
project(inspire_gripper_full)

find_package(ament_cmake REQUIRED)
find_package(bodyctrl_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_library(inspire_gripper_full SHARED inspire_gripper_full.cpp)
target_compile_features(inspire_gripper_full PRIVATE cxx_std_17)
ament_target_dependencies(
  inspire_gripper_full bodyctrl_msgs rclcpp sensor_msgs std_msgs)
target_link_libraries(
  inspire_gripper_full
  "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/libucbp.so"
  pthread)

enable_testing()
add_executable(gripper_protocol_test test_protocol.cpp)
target_compile_features(gripper_protocol_test PRIVATE cxx_std_17)
add_test(NAME gripper_protocol_test COMMAND gripper_protocol_test)

add_executable(gripper_config_test test_config.cpp)
target_compile_features(gripper_config_test PRIVATE cxx_std_17)
add_test(NAME gripper_config_test COMMAND gripper_config_test)

ament_package()

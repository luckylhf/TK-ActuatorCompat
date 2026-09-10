#!/usr/bin/env bash
set -e

source /opt/ros/humble/setup.bash
if [ -f /home/ubuntu/ros2ws/install/setup.bash ]; then
  source /home/ubuntu/ros2ws/install/setup.bash
fi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cp -a build/libinspire_gripper_full.so .

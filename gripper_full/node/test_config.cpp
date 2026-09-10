#include <cassert>
#include <cstdio>
#include <fstream>

#include "gripper_config.hpp"

int main()
{
  const char path[] = "gripper_config_test.yaml";
  {
    std::ofstream output(path);
    output << "left_gripper_installed: false\n"
           << "right_gripper_installed: true\n";
  }

  const auto installed = inspire_gripper_full::load_installed_sides(path);
  assert(!installed.left);
  assert(installed.right);

  {
    std::ofstream output(path);
    output << "left_gripper_installed: true\n"
           << "right_gripper_installed: false\n";
  }
  const auto reversed = inspire_gripper_full::load_installed_sides(path);
  assert(reversed.left);
  assert(!reversed.right);

  {
    std::ofstream output(path);
    output << "left_gripper_installed: true\n"
           << "right_gripper_installed: true\n";
  }
  const auto double_grippers = inspire_gripper_full::load_installed_sides(path);
  assert(double_grippers.left);
  assert(double_grippers.right);

  {
    std::ofstream output(path);
    output << "left_gripper_installed: false\n"
           << "right_gripper_installed: false\n";
  }
  const auto double_hands = inspire_gripper_full::load_installed_sides(path);
  assert(!double_hands.left);
  assert(!double_hands.right);

  {
    std::ofstream output(path);
    output << "left_gripper_installed: true\n";
  }
  bool rejected_missing_side = false;
  try {
    inspire_gripper_full::load_installed_sides(path);
  } catch (const std::runtime_error &) {
    rejected_missing_side = true;
  }
  assert(rejected_missing_side);
  std::remove(path);
}

#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

namespace inspire_gripper_full {

struct InstalledSides {
  bool left{};
  bool right{};
};

inline InstalledSides load_installed_sides(const std::string &path)
{
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("cannot open configuration file");
  }

  InstalledSides installed;
  bool found_left = false;
  bool found_right = false;
  std::string line;
  while (std::getline(input, line)) {
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line.erase(comment);
    }
    const auto first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
      continue;
    }
    const auto colon = line.find(':', first);
    if (colon == std::string::npos) {
      continue;
    }
    const auto key_last = line.find_last_not_of(" \t", colon - 1);
    const auto key = line.substr(first, key_last - first + 1);
    auto value_first = line.find_first_not_of(" \t", colon + 1);
    if (value_first == std::string::npos) {
      continue;
    }
    const auto value_last = line.find_last_not_of(" \t\r\n");
    const auto value = line.substr(value_first, value_last - value_first + 1);
    bool parsed;
    if (value == "true") {
      parsed = true;
    } else if (value == "false") {
      parsed = false;
    } else if (key == "left_gripper_installed" ||
               key == "right_gripper_installed") {
      throw std::runtime_error(key + " must be true or false");
    } else {
      continue;
    }
    if (key == "left_gripper_installed") {
      installed.left = parsed;
      found_left = true;
    } else if (key == "right_gripper_installed") {
      installed.right = parsed;
      found_right = true;
    }
  }

  if (!found_left || !found_right) {
    throw std::runtime_error(
        "left_gripper_installed and right_gripper_installed are required");
  }
  return installed;
}

}  // namespace inspire_gripper_full

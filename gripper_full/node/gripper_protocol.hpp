#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace inspire_gripper_full {

constexpr int kMasterIndex = 1;
constexpr int kCanChannel = 1;
constexpr std::uint16_t kLeftDeviceId = 18;
constexpr std::uint16_t kRightDeviceId = 28;

constexpr std::uint16_t kAddressCatchMode = 1005;
constexpr std::uint16_t kAddressFastStop = 1006;
constexpr std::uint16_t kAddressClearError = 1007;
constexpr std::uint16_t kAddressMotion = 1010;
constexpr std::uint16_t kAddressMaxOpen = 1016;
constexpr std::uint16_t kAddressMinOpen = 1017;
constexpr std::uint16_t kAddressStatus = 1060;
constexpr std::uint16_t kAddressError = 1064;

struct CanFrame {
  std::uint32_t id{};
  std::uint8_t size{};
  std::array<unsigned char, 8> data{};
};

struct Status {
  double position_percent{};
  double effort_percent{};
  double current_percent{};
  std::uint16_t state{};
};

inline std::uint32_t make_id(
    std::uint16_t address, std::uint16_t device_id, bool write)
{
  return (write ? (1U << 26) : 0U) |
      (static_cast<std::uint32_t>(address & 0x0FFFU) << 14) |
      (device_id & 0x3FFFU);
}

inline std::uint16_t device_id(std::uint32_t id)
{
  return static_cast<std::uint16_t>(id & 0x3FFFU);
}

inline std::uint16_t address(std::uint32_t id)
{
  return static_cast<std::uint16_t>((id >> 14) & 0x0FFFU);
}

inline std::uint16_t word(const unsigned char *data)
{
  return static_cast<std::uint16_t>(data[0]) |
      (static_cast<std::uint16_t>(data[1]) << 8);
}

inline void put_word(std::array<unsigned char, 8> &data, int offset, std::uint16_t value)
{
  data[static_cast<std::size_t>(offset)] = static_cast<unsigned char>(value);
  data[static_cast<std::size_t>(offset + 1)] = static_cast<unsigned char>(value >> 8);
}

inline std::uint16_t normalized_to_raw(double value, bool reverse)
{
  value = std::clamp(value, 0.0, 1.0);
  return static_cast<std::uint16_t>(1000.0 * (reverse ? 1.0 - value : value));
}

inline std::uint16_t percent_to_raw(double value)
{
  return static_cast<std::uint16_t>(10.0 * std::clamp(value, 0.0, 100.0));
}

inline CanFrame motion_frame(
    std::uint16_t id, double position, double velocity, double effort)
{
  CanFrame frame;
  frame.id = make_id(kAddressMotion, id, true);
  frame.size = 6;
  put_word(frame.data, 0, normalized_to_raw(position, true));
  put_word(frame.data, 2, normalized_to_raw(velocity, false));
  put_word(frame.data, 4, normalized_to_raw(effort, false));
  return frame;
}

inline CanFrame write_word_frame(
    std::uint16_t id, std::uint16_t register_address, std::uint16_t value)
{
  CanFrame frame;
  frame.id = make_id(register_address, id, true);
  frame.size = 2;
  put_word(frame.data, 0, value);
  return frame;
}

inline CanFrame read_frame(
    std::uint16_t id, std::uint16_t register_address, unsigned char response_size)
{
  CanFrame frame;
  frame.id = make_id(register_address, id, false);
  frame.size = 1;
  frame.data[0] = response_size;
  return frame;
}

inline Status parse_status(const unsigned char *data)
{
  Status status;
  status.effort_percent = static_cast<double>(word(data)) / 10.0;
  status.position_percent = 100.0 - static_cast<double>(word(data + 2)) / 10.0;
  status.current_percent = static_cast<double>(word(data + 4)) / 20.0;
  status.state = word(data + 6);
  return status;
}

inline std::uint32_t parse_error(const unsigned char *data)
{
  const auto error_bits = word(data);
  const auto state = word(data + 2);
  if (error_bits == 0) {
    return state;
  }
  if ((error_bits & 0x01U) != 0) {
    return 11;
  }
  if ((error_bits & 0x02U) != 0) {
    return 12;
  }
  if ((error_bits & 0x04U) != 0) {
    return 13;
  }
  if ((error_bits & 0x08U) != 0) {
    return 14;
  }
  if ((error_bits & 0x10U) != 0) {
    return 15;
  }
  return 16;
}

}  // namespace inspire_gripper_full

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <thread>
#include <utility>

#include <bodyctrl_msgs/msg/cmd_set_param.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/u_int32.hpp>

#include "gripper_config.hpp"
#include "gripper_protocol.hpp"

namespace {

using namespace std::chrono_literals;
using inspire_gripper_full::CanFrame;

constexpr std::size_t kMasterArrayOffset = 0x4B0;
constexpr std::size_t kMasterObjectSize = 0xE8;
constexpr std::uint32_t kDeviceLost = 33072;
constexpr char kConfigPath[] =
    "/home/ubuntu/ros2ws/install/body_control/share/body_control/param/inspire_gripper.yaml";
std::atomic<void *> g_master{nullptr};

extern "C" int gripper_full_send_raw(void *, int, int, int, unsigned char *, bool)
    asm("_ZN4ucbp13Usb2CanMaster14SendRawMessageEiiiPhb");

using RawMessageCallback = std::function<bool(int, int, int, unsigned char *)>;
extern "C" void gripper_full_register_raw(void *, RawMessageCallback)
    asm("_ZN4ucbp13Usb2CanMaster29RegisterCanRawMessageCallbackESt8functionIFbiiiPhEE");

class GripperNode : public rclcpp::Node {
public:
  GripperNode(void *master, inspire_gripper_full::InstalledSides installed)
  : Node("inspire_gripper_full", rclcpp::NodeOptions().use_global_arguments(false)),
    master_(master), installed_(installed)
  {
    if (installed_.left) {
      active_devices_[active_device_count_++] = inspire_gripper_full::kLeftDeviceId;
      left_status_ = create_publisher<sensor_msgs::msg::JointState>(
          "/inspire_gripper/left_status", 10);
      left_error_ = create_publisher<std_msgs::msg::UInt32>(
          "/inspire_gripper/left_error", 10);
      left_command_ = create_subscription<sensor_msgs::msg::JointState>(
          "/inspire_gripper/left_command", 10,
          [this](sensor_msgs::msg::JointState::SharedPtr msg) {
            on_motion(std::move(msg), inspire_gripper_full::kLeftDeviceId, "left");
          });
    }
    if (installed_.right) {
      active_devices_[active_device_count_++] = inspire_gripper_full::kRightDeviceId;
      right_status_ = create_publisher<sensor_msgs::msg::JointState>(
          "/inspire_gripper/right_status", 10);
      right_error_ = create_publisher<std_msgs::msg::UInt32>(
          "/inspire_gripper/right_error", 10);
      right_command_ = create_subscription<sensor_msgs::msg::JointState>(
          "/inspire_gripper/right_command", 10,
          [this](sensor_msgs::msg::JointState::SharedPtr msg) {
            on_motion(std::move(msg), inspire_gripper_full::kRightDeviceId, "right");
          });
    }
    clear_error_command_ = create_subscription<std_msgs::msg::String>(
        "/inspire_gripper/clear_error_command", 10,
        [this](std_msgs::msg::String::SharedPtr msg) {on_clear_error(std::move(msg));});
    catch_mode_command_ = create_subscription<bodyctrl_msgs::msg::CmdSetParam>(
        "/inspire_gripper/set_catch_mode_command", 10,
        [this](bodyctrl_msgs::msg::CmdSetParam::SharedPtr msg) {
          on_binary_param(std::move(msg), inspire_gripper_full::kAddressCatchMode, "catch mode");
        });
    fast_stop_command_ = create_subscription<bodyctrl_msgs::msg::CmdSetParam>(
        "/inspire_gripper/fast_stop_command", 10,
        [this](bodyctrl_msgs::msg::CmdSetParam::SharedPtr msg) {
          on_binary_param(std::move(msg), inspire_gripper_full::kAddressFastStop, "fast stop");
        });
    max_open_command_ = create_subscription<bodyctrl_msgs::msg::CmdSetParam>(
        "/inspire_gripper/set_max_openlen_command", 10,
        [this](bodyctrl_msgs::msg::CmdSetParam::SharedPtr msg) {
          on_open_limit(std::move(msg), inspire_gripper_full::kAddressMaxOpen, "max open length");
        });
    min_open_command_ = create_subscription<bodyctrl_msgs::msg::CmdSetParam>(
        "/inspire_gripper/set_min_openlen_command", 10,
        [this](bodyctrl_msgs::msg::CmdSetParam::SharedPtr msg) {
          on_open_limit(std::move(msg), inspire_gripper_full::kAddressMinOpen, "min open length");
        });

    const auto started = steady_now();
    last_seen_[0].store(started);
    last_seen_[1].store(started);
    lost_published_[0].store(false);
    lost_published_[1].store(false);

    gripper_full_register_raw(
        master_,
        [this](int channel, int can_id, int size, unsigned char *data) {
          return on_raw_message(channel, can_id, size, data);
        });
    poll_timer_ = create_wall_timer(20ms, [this]() {poll();});
    RCLCPP_INFO(
        get_logger(),
        "ready on Usb2Can master index %d, channel %d; left=%s right=%s",
        inspire_gripper_full::kMasterIndex, inspire_gripper_full::kCanChannel,
        installed_.left ? "enabled" : "disabled",
        installed_.right ? "enabled" : "disabled");
  }

private:
  static std::int64_t steady_now()
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
  }

  bool known_device(std::uint16_t id) const
  {
    return (installed_.left && id == inspire_gripper_full::kLeftDeviceId) ||
        (installed_.right && id == inspire_gripper_full::kRightDeviceId);
  }

  int side_index(std::uint16_t id) const
  {
    return id == inspire_gripper_full::kLeftDeviceId ? 0 : 1;
  }

  const char *side_name(std::uint16_t id) const
  {
    return id == inspire_gripper_full::kLeftDeviceId ?
        "inspire_gripper_left" : "inspire_gripper_right";
  }

  void send(const CanFrame &frame, bool log_command)
  {
    auto data = frame.data;
    const int result = gripper_full_send_raw(
        master_, inspire_gripper_full::kCanChannel,
        static_cast<int>(frame.id), frame.size, data.data(), true);
    if (log_command) {
      RCLCPP_INFO(
          get_logger(), "CAN command id=0x%08x size=%u return=%d",
          frame.id, static_cast<unsigned int>(frame.size), result);
    }
  }

  void on_motion(
      sensor_msgs::msg::JointState::SharedPtr msg,
      std::uint16_t device, const char *side)
  {
    if (msg->position.size() != 1 || msg->velocity.size() != 1 || msg->effort.size() != 1) {
      RCLCPP_ERROR(
          get_logger(), "%s command requires one position, velocity and effort value", side);
      return;
    }
    if (!std::isfinite(msg->position[0]) || !std::isfinite(msg->velocity[0]) ||
        !std::isfinite(msg->effort[0])) {
      RCLCPP_ERROR(get_logger(), "%s command contains a non-finite value", side);
      return;
    }
    send(
        inspire_gripper_full::motion_frame(
          device, msg->position[0], msg->velocity[0], msg->effort[0]),
        true);
  }

  void on_clear_error(std_msgs::msg::String::SharedPtr msg)
  {
    if (installed_.left && (msg->data == "left" || msg->data == "all")) {
      send(inspire_gripper_full::write_word_frame(
          inspire_gripper_full::kLeftDeviceId,
          inspire_gripper_full::kAddressClearError, 1), true);
    }
    if (installed_.right && (msg->data == "right" || msg->data == "all")) {
      send(inspire_gripper_full::write_word_frame(
          inspire_gripper_full::kRightDeviceId,
          inspire_gripper_full::kAddressClearError, 1), true);
    }
    if (msg->data != "left" && msg->data != "right" && msg->data != "all") {
      RCLCPP_ERROR(get_logger(), "clear_error_command must be left, right or all");
    }
  }

  void on_binary_param(
      bodyctrl_msgs::msg::CmdSetParam::SharedPtr msg,
      std::uint16_t register_address, const char *description)
  {
    for (const auto &command : msg->cmds) {
      if (!known_device(command.name)) {
        RCLCPP_WARN(
            get_logger(), "ignoring %s for unknown device id %u",
            description, static_cast<unsigned int>(command.name));
        continue;
      }
      if (!std::isfinite(command.param) ||
          (command.param != 0.0F && command.param != 1.0F)) {
        RCLCPP_ERROR(get_logger(), "%s must be 0 or 1", description);
        continue;
      }
      send(inspire_gripper_full::write_word_frame(
          command.name, register_address,
          static_cast<std::uint16_t>(command.param)), true);
    }
  }

  void on_open_limit(
      bodyctrl_msgs::msg::CmdSetParam::SharedPtr msg,
      std::uint16_t register_address, const char *description)
  {
    for (const auto &command : msg->cmds) {
      if (!known_device(command.name)) {
        RCLCPP_WARN(
            get_logger(), "ignoring %s for unknown device id %u",
            description, static_cast<unsigned int>(command.name));
        continue;
      }
      if (!std::isfinite(command.param)) {
        RCLCPP_ERROR(get_logger(), "%s contains a non-finite value", description);
        continue;
      }
      send(inspire_gripper_full::write_word_frame(
          command.name, register_address,
          inspire_gripper_full::percent_to_raw(command.param)), true);
    }
  }

  bool on_raw_message(int channel, int can_id, int size, unsigned char *data)
  {
    if (channel != inspire_gripper_full::kCanChannel || data == nullptr) {
      return false;
    }
    const auto id = static_cast<std::uint32_t>(can_id) & 0x1FFFFFFFU;
    const auto device = inspire_gripper_full::device_id(id);
    if (!known_device(device)) {
      return false;
    }

    const auto register_address = inspire_gripper_full::address(id);
    if (register_address == inspire_gripper_full::kAddressStatus) {
      if (size != 8) {
        RCLCPP_WARN(
            get_logger(), "status response for device %u has size %d",
            static_cast<unsigned int>(device), size);
        return true;
      }
      const auto status = inspire_gripper_full::parse_status(data);
      sensor_msgs::msg::JointState msg;
      msg.header.stamp = now();
      msg.name.push_back(side_name(device));
      msg.position.push_back(status.position_percent);
      msg.velocity.push_back(0.0);
      msg.effort.push_back(status.effort_percent);
      if (device == inspire_gripper_full::kLeftDeviceId) {
        left_status_->publish(msg);
      } else {
        right_status_->publish(msg);
      }
      mark_seen(device);
      return true;
    }

    if (register_address == inspire_gripper_full::kAddressError) {
      if (size != 4) {
        RCLCPP_WARN(
            get_logger(), "error response for device %u has size %d",
            static_cast<unsigned int>(device), size);
        return true;
      }
      publish_error(device, inspire_gripper_full::parse_error(data));
      mark_seen(device);
      return true;
    }
    return false;
  }

  void mark_seen(std::uint16_t device)
  {
    const int side = side_index(device);
    last_seen_[static_cast<std::size_t>(side)].store(steady_now());
    lost_published_[static_cast<std::size_t>(side)].store(false);
  }

  void publish_error(std::uint16_t device, std::uint32_t value)
  {
    std_msgs::msg::UInt32 msg;
    msg.data = value;
    if (device == inspire_gripper_full::kLeftDeviceId) {
      left_error_->publish(msg);
    } else {
      right_error_->publish(msg);
    }
  }

  void poll()
  {
    const auto device = active_devices_[(poll_step_ / 2) % active_device_count_];
    if ((poll_step_ % 2) == 0) {
      send(inspire_gripper_full::read_frame(
          device, inspire_gripper_full::kAddressStatus, 8), false);
    } else {
      send(inspire_gripper_full::read_frame(
          device, inspire_gripper_full::kAddressError, 4), false);
    }
    poll_step_ = (poll_step_ + 1) % (active_device_count_ * 2);

    const auto current = steady_now();
    const std::array<std::uint16_t, 2> devices = {
      inspire_gripper_full::kLeftDeviceId,
      inspire_gripper_full::kRightDeviceId};
    for (std::size_t side = 0; side < devices.size(); ++side) {
      if ((side == 0 && !installed_.left) || (side == 1 && !installed_.right)) {
        continue;
      }
      if (current - last_seen_[side].load() > 2000000000LL &&
          !lost_published_[side].exchange(true)) {
        publish_error(devices[side], kDeviceLost);
      }
    }
  }

  void *master_;
  inspire_gripper_full::InstalledSides installed_;
  std::array<std::uint16_t, 2> active_devices_{};
  std::size_t active_device_count_{0};
  std::size_t poll_step_{0};
  std::array<std::atomic<std::int64_t>, 2> last_seen_{};
  std::array<std::atomic<bool>, 2> lost_published_{};
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr left_status_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr right_status_;
  rclcpp::Publisher<std_msgs::msg::UInt32>::SharedPtr left_error_;
  rclcpp::Publisher<std_msgs::msg::UInt32>::SharedPtr right_error_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr left_command_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr right_command_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr clear_error_command_;
  rclcpp::Subscription<bodyctrl_msgs::msg::CmdSetParam>::SharedPtr catch_mode_command_;
  rclcpp::Subscription<bodyctrl_msgs::msg::CmdSetParam>::SharedPtr fast_stop_command_;
  rclcpp::Subscription<bodyctrl_msgs::msg::CmdSetParam>::SharedPtr max_open_command_;
  rclcpp::Subscription<bodyctrl_msgs::msg::CmdSetParam>::SharedPtr min_open_command_;
  rclcpp::TimerBase::SharedPtr poll_timer_;
};

std::shared_ptr<GripperNode> g_node;

void *run_node(void *)
{
  while (!rclcpp::ok()) {
    std::this_thread::sleep_for(100ms);
  }
  while (rclcpp::ok() && g_master.load() == nullptr) {
    std::this_thread::sleep_for(100ms);
  }
  if (!rclcpp::ok()) {
    return nullptr;
  }

  try {
    const auto installed = inspire_gripper_full::load_installed_sides(kConfigPath);
    if (!installed.left && !installed.right) {
      std::fprintf(stderr, "inspire_gripper_full: no gripper side is enabled\n");
      return nullptr;
    }
    g_node = std::make_shared<GripperNode>(g_master.load(), installed);
  } catch (const std::exception &error) {
    std::fprintf(
        stderr, "inspire_gripper_full: cannot load %s: %s\n",
        kConfigPath, error.what());
    return nullptr;
  }
  rclcpp::spin(g_node);
  return nullptr;
}

__attribute__((constructor)) void start_node_thread()
{
  pthread_t thread;
  if (pthread_create(&thread, nullptr, run_node, nullptr) == 0) {
    pthread_detach(thread);
  }
}

}  // namespace

extern "C" void *gripper_full_get_u2c_master(void *body_control, int index)
    asm("_ZN12body_control11BodyControl12GetU2CMasterEi");

extern "C" void *gripper_full_get_u2c_master(void *body_control, int index)
{
  void *master = static_cast<unsigned char *>(body_control) +
      kMasterArrayOffset + static_cast<std::size_t>(index) * kMasterObjectSize;
  if (index == inspire_gripper_full::kMasterIndex) {
    g_master.store(master);
  }
  return master;
}

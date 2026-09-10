#include <cassert>
#include <cmath>
#include <cstdint>

#include "gripper_protocol.hpp"

using namespace inspire_gripper_full;

int main()
{
  const auto close = motion_frame(kLeftDeviceId, 1.0, 0.3, 0.3);
  assert(close.id == 0x04FC8012U);
  assert(close.size == 6);
  assert(close.data[0] == 0x00 && close.data[1] == 0x00);
  assert(close.data[2] == 0x2C && close.data[3] == 0x01);
  assert(close.data[4] == 0x2C && close.data[5] == 0x01);

  const auto open = motion_frame(kRightDeviceId, 0.0, 0.3, 0.3);
  assert(open.id == 0x04FC801CU);
  assert(open.data[0] == 0xE8 && open.data[1] == 0x03);

  const auto status_request = read_frame(kLeftDeviceId, kAddressStatus, 8);
  assert(status_request.id == 0x01090012U);
  assert(status_request.size == 1 && status_request.data[0] == 8);
  const auto error_request = read_frame(kRightDeviceId, kAddressError, 4);
  assert(error_request.id == 0x010A001CU);
  assert(error_request.size == 1 && error_request.data[0] == 4);

  const auto mode = write_word_frame(kLeftDeviceId, kAddressCatchMode, 1);
  assert(mode.id == 0x04FB4012U);
  assert(mode.size == 2 && mode.data[0] == 1 && mode.data[1] == 0);
  assert(percent_to_raw(35.5) == 355);

  const unsigned char status_data[8] = {
    0xFA, 0x00, 0xEE, 0x02, 0xE8, 0x03, 0x06, 0x00};
  const auto status = parse_status(status_data);
  assert(std::abs(status.position_percent - 25.0) < 0.001);
  assert(std::abs(status.effort_percent - 25.0) < 0.001);
  assert(std::abs(status.current_percent - 50.0) < 0.001);
  assert(status.state == 6);

  for (std::uint16_t state = 0; state <= 6; ++state) {
    const unsigned char no_error[4] = {
      0x00, 0x00, static_cast<unsigned char>(state), 0x00};
    assert(parse_error(no_error) == state);
  }
  const unsigned char locked_rotor[4] = {0x01, 0x00, 0x01, 0x00};
  assert(parse_error(locked_rotor) == 11);
  const unsigned char over_temperature[4] = {0x02, 0x00, 0x02, 0x00};
  assert(parse_error(over_temperature) == 12);
  const unsigned char over_current[4] = {0x04, 0x00, 0x02, 0x00};
  assert(parse_error(over_current) == 13);
  const unsigned char running_fault[4] = {0x08, 0x00, 0x03, 0x00};
  assert(parse_error(running_fault) == 14);
  const unsigned char communication_fault[4] = {0x10, 0x00, 0x03, 0x00};
  assert(parse_error(communication_fault) == 15);
  const unsigned char unknown_error[4] = {0x20, 0x00, 0x03, 0x00};
  assert(parse_error(unknown_error) == 16);
}

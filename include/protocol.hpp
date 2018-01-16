#ifndef _PROTOCOL_HPP_
#define _PROTOCOL_HPP_

/*
 * Device communication protocol specifics.
 * Implementation detail; not exported.
 */

#include <stdint.h>

#include "error.hpp"
#include "serial.hpp"
#include "neo.h"

namespace neo {
namespace protocol {

struct error : neo::error::error {
  using base = neo::error::error;
  using base::base;
};

// Command Symbols

constexpr uint8_t DATA_ACQUISITION_START[2] = {'D', 'S'};
constexpr uint8_t DATA_ACQUISITION_STOP[2]  = {'D', 'X'};
constexpr uint8_t MOTOR_SPEED_ADJUST[2]     = {'M', 'S'};
constexpr uint8_t MOTOR_INFORMATION[2]      = {'M', 'I'};
// constexpr uint8_t SAMPLE_RATE_ADJUST[2]     = {'L', 'R'};
// constexpr uint8_t SAMPLE_RATE_INFORMATION[2]= {'L', 'I'};
constexpr uint8_t VERSION_INFORMATION[2]    = {'I', 'V'};
constexpr uint8_t DEVICE_INFORMATION[2]     = {'I', 'D'};
constexpr uint8_t RESET_DEVICE[2]           = {'R', 'R'};
constexpr uint8_t DEVICE_CALIBRATION[2]     = {'C', 'S'};

// Packets for communication

// Make in-memory representations correspond to bytes we send over the wire.
#pragma pack(push, 1)

struct cmd_packet_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamTerm;
};

static_assert(sizeof(cmd_packet_s) == 3, "cmd packet size mismatch.");

struct cmd_param_packet_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamByte1;
  uint8_t cmdParamByte2;
  uint8_t cmdParamTerm;
};

static_assert(sizeof(cmd_param_packet_s) == 5,
    "cmd param packet size mismatch.");

struct response_header_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdStatusByte1;
  uint8_t cmdStatusByte2;
  uint8_t cmdSum;
  uint8_t term1;
};

static_assert(sizeof(response_header_s) == 6, "response header size mismatch.");

struct response_param_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamByte1;
  uint8_t cmdParamByte2;
  uint8_t term1;
  uint8_t cmdStatusByte1;
  uint8_t cmdStatusByte2;
  uint8_t cmdSum;
  uint8_t term2;
};

static_assert(sizeof(response_param_s) == 9, "response param size mismatch.");

struct response_scan_packet_s {
  // +  -  -  -  -  -  -  -  -  -  - +
  // | Distance[4:0] | VHL | S2 | S1 |
  // |     Distance[12:5]            |
  uint8_t  s1:1;
  uint8_t  s2:1;
  uint8_t  VHL:1;
  uint8_t  distance_low:5;
  uint8_t  distance_high:8;
  uint16_t angle;
  uint8_t  checksum:4;
  uint8_t  VRECT:4;
};

static_assert(sizeof(response_scan_packet_s) == 5,
    "response scan packet size mismatch.");

namespace response_scan_packet_sync {
enum bits : uint8_t {
  sync = 1 << 0,                 // beginning of new full scan
  communication_error = 1 << 1,  // communication error

  // Reserved for future error bits
  reserved2 = 1 << 2,
  reserved3 = 1 << 3,
  reserved4 = 1 << 4,
  reserved5 = 1 << 5,
  reserved6 = 1 << 6,
  reserved7 = 1 << 7,
};
}  // namespace response_scan_packet_sync

struct response_info_device_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t bit_rate[6];
  uint8_t laser_state;
  uint8_t mode;
  uint8_t diagnostic;
  uint8_t motor_speed[2];
  uint8_t sample_rate[4];
  uint8_t term;
};

static_assert(sizeof(response_info_device_s) == 18,
    "response info device size mismatch.");

struct response_info_version_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t model[5];
  uint8_t protocol_major;
  uint8_t protocol_min;
  uint8_t firmware_major;
  uint8_t firmware_minor;
  uint8_t hardware_version;
  uint8_t serial_no[8];
  uint8_t term;
};

static_assert(sizeof(response_info_version_s) == 21,
    "response info version size mismatch.");

struct response_info_motor_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t motor_speed[2];
  uint8_t term;
};

static_assert(sizeof(response_info_motor_s) == 5,
    "response info motor size mismatch.");

// Done with in-memory representations for packets we send over the wire.
#pragma pack(pop)

// Read and write specific packets

void write_command(neo::serial::device_s serial, const uint8_t cmd[2]);

void write_command_with_arguments(neo::serial::device_s serial,
    const uint8_t cmd[2], const uint8_t arg[2]);

response_header_s read_response_header(neo::serial::device_s serial,
    const uint8_t cmd[2]);

response_param_s read_response_param(neo::serial::device_s serial,
    const uint8_t cmd[2]);

response_scan_packet_s read_response_scan(neo::serial::device_s serial);

response_info_motor_s read_response_info_motor(neo::serial::device_s serial);

inline void integral_to_ascii_bytes(const int32_t integral, uint8_t bytes[2]) {
  NEO_ASSERT(integral >= 0);
  NEO_ASSERT(integral <= 99);
  NEO_ASSERT(bytes);

  // Numbers begin at ASCII code point 48
  const uint8_t ASCIINumberBlockOffset = '0';

  const uint8_t num1 = (integral / 10) + ASCIINumberBlockOffset;
  const uint8_t num2 = (integral % 10) + ASCIINumberBlockOffset;

  bytes[0] = num1;
  bytes[1] = num2;
}

inline int32_t ascii_bytes_to_integral(const uint8_t bytes[2]) {
  NEO_ASSERT(bytes);

  // Numbers begin at ASCII code point 48
  const uint8_t ASCIINumberBlockOffset = '0';

  NEO_ASSERT(bytes[0] >= ASCIINumberBlockOffset);
  NEO_ASSERT(bytes[1] >= ASCIINumberBlockOffset);

  const uint8_t num1 = static_cast<uint8_t>(bytes[0] - ASCIINumberBlockOffset);
  const uint8_t num2 = static_cast<uint8_t>(bytes[1] - ASCIINumberBlockOffset);

  NEO_ASSERT(num1 <= 9);
  NEO_ASSERT(num2 <= 9);

  const int32_t integral = (num1 * 10) + (num2 * 1);

  NEO_ASSERT(integral >= 0);
  NEO_ASSERT(integral <= 99);

  return integral;
}

}  // namespace protocol
}  // namespace neo

#endif  // _PROTOCOL_HPP_

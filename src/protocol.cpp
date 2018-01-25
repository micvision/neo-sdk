#include <chrono>
#include <thread>
#include <stdio.h>
#include <stdlib.h>

#include "protocol.hpp"

namespace neo {
namespace protocol {

static uint8_t checksum_response_header(const response_header_s& v) {
  return ((v.cmdStatusByte1 + v.cmdStatusByte2) & 0x3F) + 0x30;
}

static uint8_t checksum_response_param(const response_param_s& v) {
  return ((v.cmdStatusByte1 + v.cmdStatusByte2) & 0x3F) + 0x30;
}

static uint8_t checksum_response_scan_packet(const response_scan_packet_s& v) {
  uint64_t checksum = 0;
  checksum += (v.distance_low << 3) + (v.VHL << 2) + (v.s2 << 1) + v.s1;
  checksum += v.distance_high;
  checksum += v.angle & 0x00ff;
  checksum += v.angle >> 8;
  checksum += v.VRECT << 4;
  return checksum % 15;
}

void write_command(serial::device_s serial, const uint8_t cmd[2]) {
  NEO_ASSERT(serial);
  NEO_ASSERT(cmd);

  cmd_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamTerm = '\n';

  std::this_thread::sleep_for(std::chrono::milliseconds(2));

  serial::device_write(serial, &packet, sizeof(cmd_packet_s));
}

void write_command_with_arguments(serial::device_s serial,
    const uint8_t cmd[2], const uint8_t arg[2]) {
  NEO_ASSERT(serial);
  NEO_ASSERT(cmd);
  NEO_ASSERT(arg);

  cmd_param_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamByte1 = arg[0];
  packet.cmdParamByte2 = arg[1];
  packet.cmdParamTerm = '\n';

  serial::device_write(serial, &packet, sizeof(cmd_param_packet_s));
}

response_header_s read_response_header(serial::device_s serial,
    const uint8_t cmd[2]) {
  NEO_ASSERT(serial);
  NEO_ASSERT(cmd);

  response_header_s header;

  serial::device_read(serial, &header, sizeof(response_header_s));

  uint8_t checksum = checksum_response_header(header);

  if ( checksum != header.cmdSum ) {
    throw error{"invalid response header checksum."};
  }

  bool ok = header.cmdByte1 == cmd[0] && header.cmdByte2 == cmd[1];

  if (!ok) {
    throw error{"invalid header response commands."};
  }

  return header;
}

response_param_s read_response_param(serial::device_s serial,
    const uint8_t cmd[2]) {
  NEO_ASSERT(serial);
  NEO_ASSERT(cmd);

  response_param_s param;
  serial::device_read(serial, &param, sizeof(response_param_s));

  uint8_t checksum = checksum_response_param(param);

  if ( checksum != param.cmdSum ) {
    throw error{"invalid response param header checksum."};
  }

  bool ok = param.cmdByte1 == cmd[0] && param.cmdByte2 == cmd[1];

  if ( !ok ) {
    throw error{"invalid param response commands."};
  }
  return param;
}

response_scan_packet_s read_response_scan(serial::device_s serial) {
  NEO_ASSERT(serial);

  response_scan_packet_s scan;
  serial::device_read(serial, &scan, sizeof(response_scan_packet_s));

  uint8_t checksum = checksum_response_scan_packet(scan);

  // checksum error handle.
  unsigned int i = 0;
  char *p2scan = (char*)&scan;
  char *p2scan_back = p2scan+sizeof(response_scan_packet_s)-1;
  unsigned short error_count = 0; // error count.
  while ( checksum != scan.checksum && error_count<100) {
    for (i = 0; i < sizeof(response_scan_packet_s)-1; ++i) {
        p2scan[i] = p2scan[i+1];
    }
    error_count++;
    serial::device_read(serial, (void *) p2scan_back, 1);
    checksum = checksum_response_scan_packet(scan);
    if(checksum == scan.checksum)
    {
      error_count += 5;
      serial::device_read(serial, &scan, sizeof(response_scan_packet_s));
      checksum = checksum_response_scan_packet(scan);
    }
  }
  // checksum error handle end.

  if ( checksum != scan.checksum ) {
    throw error{"invalid scan response commands."};
  }
  return scan;
}

response_info_motor_s read_response_info_motor(serial::device_s serial) {
  NEO_ASSERT(serial);

  response_info_motor_s info;
  serial::device_read(serial, &info, sizeof(response_info_motor_s));

  bool ok = info.cmdByte1 == MOTOR_INFORMATION[0] &&
    info.cmdByte2 == MOTOR_INFORMATION[1];

  if ( !ok ) {
    throw error{"invalid motor info response commands."};
  }
  return info;
}

}  // namespace protocol
}  // namespace neo

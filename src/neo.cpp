#include <stdio.h>
#include "neo.h"
#include "protocol.h"
#include "serial.h"

#include <chrono>
#include <thread>
#include <algorithm>

int32_t neo_get_version(void) { return NEO_VERSION; }
bool neo_is_abi_compatible(void) { return neo_get_version() >> 16u == NEO_VERSION_MAJOR; }

typedef struct neo_error {
  const char* what; // always literal, do not deallocate
} neo_error;

typedef struct neo_device {
  neo::serial::device_s serial; // serial port communication
  bool is_scanning;
} neo_device;

#define NEO_MAX_SAMPLES 4096

typedef struct sample {
  float angle;             // in millidegrees
  int32_t distance;        // in cm
  // int32_t signal_strength[NEO_MAX_SAMPLES]; // range 0:255
} sample;

typedef struct neo_scan {
  sample samples[NEO_MAX_SAMPLES];
  int32_t count;
} neo_scan;

static sample parse_payload(const neo::protocol::response_scan_packet_s &msg) {
  sample ret;
  ret.angle = (float)msg.angle * 7.8125f;  // 7.8125f = 1000.0f / 128.0f
  ret.distance = ((int32_t)msg.distance_low)
    + ((int32_t)msg.distance_high << 5);

  return ret;
}

// Constructor hidden from users
static neo_error_s neo_error_construct(const char* what) {
  NEO_ASSERT(what);

  auto out = new neo_error{what};
  return out;
}

const char* neo_error_message(neo_error_s error) {
  NEO_ASSERT(error);

  return error->what;
}

void neo_error_destruct(neo_error_s error) {
  NEO_ASSERT(error);

  delete error;
}

neo_device_s neo_device_construct_simple(const char* port, neo_error_s* error) {
  NEO_ASSERT(error);

  return neo_device_construct(port, 115200, error);
}

neo_device_s neo_device_construct(const char* port, int32_t bitrate, neo_error_s* error) {
  NEO_ASSERT(port);
  NEO_ASSERT(bitrate > 0);
  NEO_ASSERT(error);

  neo::serial::error_s serialerror = nullptr;
  neo::serial::device_s serial = neo::serial::device_construct(port, bitrate, &serialerror);

  if (serialerror) {
    *error = neo_error_construct(neo::serial::error_message(serialerror));
    neo::serial::error_destruct(serialerror);
    return nullptr;
  }

  auto out = new neo_device{serial, /*is_scanning=*/false};

  // Stop all process to recovery
  out->is_scanning = true;
  neo_error_s stoperror = nullptr;
  neo_device_stop_scanning(out, &stoperror);

  // Setting motor running
  neo_error_s MS05_error = nullptr;
  neo_device_set_motor_speed(out, 5, &MS05_error);
  if (MS05_error) {
    *error = MS05_error;
    neo_device_destruct(out);
    return nullptr;
  }

  printf("Wait the motor speed stabilizes...\n");
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // device calibration
  neo_error_s CS_error = nullptr;
  neo_device_calibrate(out, &CS_error);
  if (CS_error) {
    *error = CS_error;
    neo_device_destruct(out);
    return nullptr;
  }

  // Stop motor
  neo_error_s MS00_error = nullptr;
  neo_device_set_motor_speed(out, 0, &MS00_error);
  if (MS00_error) {
    *error = MS00_error;
    neo_device_destruct(out);
    return nullptr;
  }

  neo_device_stop_scanning(out, &stoperror);

  if (stoperror) {
    *error = stoperror;
    neo_device_destruct(out);
    return nullptr;
  }

  return out;
}

void neo_device_destruct(neo_device_s device) {
  NEO_ASSERT(device);

  neo_error_s ignore = nullptr;
  neo_device_stop_scanning(device, &ignore);
  (void)ignore; // nothing we can do here

  neo::serial::device_destruct(device->serial);

  delete device;
}

void neo_device_start_scanning(neo_device_s device, neo_error_s* error) {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  if (device->is_scanning)
    return;

  neo::protocol::error_s protocolerror = nullptr;
  neo::protocol::write_command(device->serial, neo::protocol::DATA_ACQUISITION_START, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to send start scanning command");
    neo::protocol::error_destruct(protocolerror);
    return;
  }

  neo::protocol::response_header_s response;
  neo::protocol::read_response_header(device->serial, neo::protocol::DATA_ACQUISITION_START, &response, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to receive start scanning command response");
    neo::protocol::error_destruct(protocolerror);
    return;
  }

  device->is_scanning = true;
}

void neo_device_stop_scanning(neo_device_s device, neo_error_s* error) {
  NEO_ASSERT(device);
  NEO_ASSERT(error);

  if (!device->is_scanning)
    return;

  neo::protocol::error_s protocolerror = nullptr;
  neo::protocol::write_command(device->serial, neo::protocol::DATA_ACQUISITION_STOP, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to send stop scanning command");
    neo::protocol::error_destruct(protocolerror);
    return;
  }

  // Wait until device stopped sending
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  neo::serial::error_s serialerror = nullptr;
  neo::serial::device_flush(device->serial, &serialerror);

  if (serialerror) {
    *error = neo_error_construct("unable to flush serial device for stopping scanning command");
    neo::serial::error_destruct(serialerror);
    return;
  }

  neo::protocol::write_command(device->serial, neo::protocol::DATA_ACQUISITION_STOP, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to send stop scanning command");
    neo::protocol::error_destruct(protocolerror);
    return;
  }

  neo::protocol::response_header_s response;
  neo::protocol::read_response_header(device->serial, neo::protocol::DATA_ACQUISITION_STOP, &response, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to receive stop scanning command response");
    neo::protocol::error_destruct(protocolerror);
    return;
  }

  device->is_scanning = false;
}

neo_scan_s neo_device_get_scan(neo_device_s device, neo_error_s* error) {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(device->is_scanning);

  neo::protocol::error_s protocolerror = nullptr;

  // neo::protocol::response_scan_packet_s responses[NEO_MAX_SAMPLES];
  static neo::protocol::response_scan_packet_s responses_syns;
  neo::protocol::response_scan_packet_s response;

  int32_t received = 0;
  sample buffer[NEO_MAX_SAMPLES];

  while (received < NEO_MAX_SAMPLES) {
    neo::protocol::read_response_scan(device->serial, &response, &protocolerror);

    if (protocolerror) {
      *error = neo_error_construct("unable to receive neo scan response");
      neo::protocol::error_destruct(protocolerror);
      return nullptr;
    }

    if ( 0 == received ) {
      buffer[received++] = parse_payload(responses_syns);
      buffer[received++] = parse_payload(response);
      continue;
    }

    const bool is_sync = response.s1
      & neo::protocol::response_scan_packet_sync::sync;

    if ( is_sync ) {
      responses_syns = response;
      break;
    }

    buffer[received] = parse_payload(response);
    received++;

  }

  auto out = new neo_scan;

  out->count = received - 1;
  std::copy_n(std::begin(buffer), received - 1, std::begin(out->samples));

  return out;
}

int32_t neo_scan_get_number_of_samples(neo_scan_s scan) {
  NEO_ASSERT(scan);
  NEO_ASSERT(scan->count >= 0);

  return scan->count;
}

float neo_scan_get_angle(neo_scan_s scan, int32_t sample) {
  NEO_ASSERT(scan);
  NEO_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->samples[sample].angle;
}

int32_t neo_scan_get_distance(neo_scan_s scan, int32_t sample) {
  NEO_ASSERT(scan);
  NEO_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->samples[sample].distance;
}

/*
int32_t neo_scan_get_signal_strength(neo_scan_s scan, int32_t sample) {
  NEO_ASSERT(scan);
  NEO_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->signal_strength[sample];
}
*/

void neo_scan_destruct(neo_scan_s scan) {
  NEO_ASSERT(scan);

  delete scan;
}

int32_t neo_device_get_motor_speed(neo_device_s device, neo_error_s* error) {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  neo::protocol::error_s protocolerror = nullptr;

  neo::protocol::write_command(device->serial, neo::protocol::MOTOR_INFORMATION, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to send motor speed command");
    neo::protocol::error_destruct(protocolerror);
    return 0;
  }

  neo::protocol::response_info_motor_s response;
  neo::protocol::read_response_info_motor(device->serial, neo::protocol::MOTOR_INFORMATION, &response, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to receive motor speed command response");
    neo::protocol::error_destruct(protocolerror);
    return 0;
  }

  int32_t speed = neo::protocol::ascii_bytes_to_integral(response.motor_speed);
  NEO_ASSERT(speed >= 0);

  return speed;
}

void neo_device_set_motor_speed(neo_device_s device, int32_t hz, neo_error_s* error) {
  NEO_ASSERT(device);
  NEO_ASSERT(hz >= 0 && hz <= 10);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  uint8_t args[2] = {0};
  neo::protocol::integral_to_ascii_bytes(hz, args);

  neo::protocol::error_s protocolerror = nullptr;

  neo::protocol::write_command_with_arguments(device->serial, neo::protocol::MOTOR_SPEED_ADJUST, args, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to send motor speed command");
    neo::protocol::error_destruct(protocolerror);
    return;
  }

  neo::protocol::response_param_s response;
  neo::protocol::read_response_param(device->serial, neo::protocol::MOTOR_SPEED_ADJUST, &response, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to receive motor speed command response");
    neo::protocol::error_destruct(protocolerror);
    return;
  }
}

int32_t neo_device_get_sample_rate(neo_device_s device, neo_error_s* error) {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  neo::protocol::error_s protocolerror = nullptr;

  neo::protocol::write_command(device->serial, neo::protocol::SAMPLE_RATE_INFORMATION, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to send sample rate command");
    neo::protocol::error_destruct(protocolerror);
    return 0;
  }

  neo::protocol::response_info_sample_rate_s response;
  neo::protocol::read_response_info_sample_rate(device->serial, neo::protocol::SAMPLE_RATE_INFORMATION, &response,
                                                  &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to receive sample rate command response");
    neo::protocol::error_destruct(protocolerror);
    return 0;
  }

  // 01: 500-600Hz, 02: 750-800Hz, 03: 1000-1050Hz
  int32_t code = neo::protocol::ascii_bytes_to_integral(response.sample_rate);
  int32_t rate = 0;

  switch (code) {
  case 1:
    rate = 500;
    break;
  case 2:
    rate = 750;
    break;
  case 3:
    rate = 1000;
    break;
  default:
    NEO_ASSERT(false && "sample rate code unknown");
  }

  return rate;
}

void neo_device_set_sample_rate(neo_device_s device, int32_t hz, neo_error_s* error) {
  NEO_ASSERT(device);
  NEO_ASSERT(hz == 500 || hz == 750 || hz == 1000);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  // 01: 500-600Hz, 02: 750-800Hz, 03: 1000-1050Hz
  int32_t code = 1;

  switch (hz) {
  case 500:
    code = 1;
    break;
  case 750:
    code = 2;
    break;
  case 1000:
    code = 3;
    break;
  default:
    NEO_ASSERT(false && "sample rate unknown");
  }

  uint8_t args[2] = {0};
  neo::protocol::integral_to_ascii_bytes(code, args);

  neo::protocol::error_s protocolerror = nullptr;

  neo::protocol::write_command_with_arguments(device->serial, neo::protocol::SAMPLE_RATE_ADJUST, args, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to send sample rate command");
    neo::protocol::error_destruct(protocolerror);
    return;
  }

  neo::protocol::response_param_s response;
  neo::protocol::read_response_param(device->serial, neo::protocol::SAMPLE_RATE_ADJUST, &response, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to receive sample rate command response");
    neo::protocol::error_destruct(protocolerror);
    return;
  }
}

void neo_device_reset(neo_device_s device, neo_error_s* error) {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  neo::protocol::error_s protocolerror = nullptr;

  neo::protocol::write_command(device->serial, neo::protocol::RESET_DEVICE, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to send device reset command");
    neo::protocol::error_destruct(protocolerror);
    return;
  }
}
void neo_device_calibrate(neo_device_s device, neo_error_s* error) {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  neo::protocol::error_s protocolerror = nullptr;
  neo::protocol::write_command(device->serial,
      neo::protocol::DEVICE_CALIBRATION, &protocolerror);

  if (protocolerror) {
    *error = neo_error_construct("unable to send device calibration command");
    neo::protocol::error_destruct(protocolerror);
    return;
  }

  printf("wait device calibration...  \n");


  neo::protocol::response_header_s response;
  neo::protocol::read_response_header(device->serial,
                                      neo::protocol::DEVICE_CALIBRATION,
                                      &response, &protocolerror);

  if (protocolerror) {
    *error =
      neo_error_construct("unable to receive device calibration response");
    neo::protocol::error_destruct(protocolerror);
    return;
  }

}


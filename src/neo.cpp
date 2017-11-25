#include <stdio.h>
#include "neo.h"
#include "protocol.h"
#include "serial.h"

#include <chrono>
#include <thread>

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

typedef struct neo_scan {
  int32_t angle[NEO_MAX_SAMPLES];           // in millidegrees
  int32_t distance[NEO_MAX_SAMPLES];        // in cm
  // int32_t signal_strength[NEO_MAX_SAMPLES]; // range 0:255
  int32_t count;
} neo_scan;

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

  // Setting motor running
  neo_error_s MS05_error = nullptr;
  neo_device_set_motor_speed(out, 5, &MS05_error);
  if (MS05_error) {
    *error = MS05_error;
    neo_device_destruct(out);
    return nullptr;
  }

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

  out->is_scanning = true;
  neo_error_s stoperror = nullptr;
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

  neo::protocol::response_scan_packet_s responses[NEO_MAX_SAMPLES];
  static neo::protocol::response_scan_packet_s responses_syns;

  int32_t received = 0;

  while (received < NEO_MAX_SAMPLES) {
    if (0 == received) {
      responses[received++] = responses_syns;
      continue;
    }
    neo::protocol::read_response_scan(device->serial, &responses[received], &protocolerror);

    if (protocolerror) {
      *error = neo_error_construct("unable to receive neo scan response");
      neo::protocol::error_destruct(protocolerror);
      return nullptr;
    }

    //const bool is_sync = responses[received].sync_error & neo::protocol::response_scan_packet_sync::sync;
    //const bool has_error = (responses[received].sync_error >> 1) != 0; // shift out sync bit, others are errors
    const bool is_sync = responses[received].s1
        & neo::protocol::response_scan_packet_sync::sync;

    received++;

    if (is_sync) {
      responses_syns = responses[received-1];
      break;
    }
  }

  auto out = new neo_scan;

  out->count = received - 1;

  for (int32_t it = 0; it < received - 1; ++it) {
    // Convert angle from compact serial format to float (in degrees).
    // In addition convert from degrees to milli-degrees.
    out->angle[it] = static_cast<int32_t>(neo::protocol::u16_to_f32(responses[it].angle) * 1000.f);
    out->distance[it] = ((int)responses[it].distance_low)
        + ((int)responses[it].distance_high << 5);
    //out->signal_strength[it] = responses[it].signal_strength;
  }

  return out;
}

int32_t neo_scan_get_number_of_samples(neo_scan_s scan) {
  NEO_ASSERT(scan);
  NEO_ASSERT(scan->count >= 0);

  return scan->count;
}

int32_t neo_scan_get_angle(neo_scan_s scan, int32_t sample) {
  NEO_ASSERT(scan);
  NEO_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->angle[sample];
}

int32_t neo_scan_get_distance(neo_scan_s scan, int32_t sample) {
  NEO_ASSERT(scan);
  NEO_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->distance[sample];
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

  int time = 0;
  printf("wait device calibration...  \n");
  while (time < 10) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ++time;
    printf("%d  \n", time);
  }
  printf("\n");


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


#include <stdio.h>
#include "neo.h"
#include "protocol.hpp"
#include "serial.hpp"
#include "queue.hpp"
#include "error.hpp"

#include <chrono>
#include <thread>
#include <algorithm>
#include <utility>
#include <memory>
#include <string>

int32_t neo_get_version(void) { return NEO_VERSION; }
bool neo_is_abi_compatible(void) {
  return neo_get_version() >> 16u == NEO_VERSION_MAJOR;
}

struct neo_error {
  std::string what;
};

struct neo_device {
  neo::serial::device_s serial;  // serial port communication
  bool is_scanning;

  std::atomic<bool> stop_thread;
  struct Element {
    std::unique_ptr<neo_scan> scan;
    std::exception_ptr error;
  };

  neo::queue::queue<Element> scan_queue;
};

#define NEO_MAX_SAMPLES 4096

struct sample {
  float angle;             // in degrees
  int32_t distance;        // in cm
  // int32_t signal_strength[NEO_MAX_SAMPLES]; // range 0:255
};

struct neo_scan {
  sample samples[NEO_MAX_SAMPLES];
  int32_t count;
};

static sample parse_payload(const neo::protocol::response_scan_packet_s &msg) {
  sample ret;
  ret.angle = static_cast<float>(msg.angle) / 128; // angle / 128
  ret.distance = ((int32_t)msg.distance_low)
    + ((int32_t)msg.distance_high << 5);

  return ret;
}

static void neo_device_accumulate_scans(neo_device_s device) try {
  NEO_ASSERT(device);
  NEO_ASSERT(device->is_scanning);

  sample buffer[NEO_MAX_SAMPLES];
  int32_t received = 0;

  while ( !device->stop_thread && received < NEO_MAX_SAMPLES ) {
    const auto response = neo::protocol::read_response_scan(device->serial);

    buffer[received++] = parse_payload(response);

    const bool is_sync = response.s1
      & neo::protocol::response_scan_packet_sync::sync;

     if ( received > 2
         && (is_sync || (buffer[received-2].angle > buffer[received-1].angle)) ) {
      auto out = std::unique_ptr<neo_scan>(new neo_scan);
      out->count = received - 1;
      std::copy_n(std::begin(buffer), received - 1, std::begin(out->samples));

      device ->scan_queue.enqueue({std::move(out), nullptr});

      buffer[0] = buffer[received - 1];
      received = 1;
    }
  }
} catch (...) {
  // worker thread is dead at this point
  device->scan_queue.enqueue({nullptr, std::current_exception()});
}

// Constructor hidden from users
static neo_error_s neo_error_construct(const char* what) {
  NEO_ASSERT(what);

  auto out = new neo_error{what};
  return out;
}

const char* neo_error_message(neo_error_s error) {
  NEO_ASSERT(error);

  return error->what.c_str();
}

void neo_error_destruct(neo_error_s error) {
  NEO_ASSERT(error);

  delete error;
}

neo_device_s neo_device_construct_simple(const char* port,
    neo_error_s* error) try {
  NEO_ASSERT(error);

  return neo_device_construct(port, 115200, error);
} catch ( const std::exception& e ) {
  *error = neo_error_construct(e.what());
  return nullptr;
}

neo_device_s neo_device_construct(const char* port, int32_t baudrate,
    neo_error_s* error) try {
  NEO_ASSERT(port);
  NEO_ASSERT(baudrate > 0);
  NEO_ASSERT(error);

  neo::serial::device_s serial = neo::serial::device_construct(port, baudrate);

  auto out = new neo_device{serial, /*is_scanning=*/true,
  /*stop_thread=*/{false}, /*scan_queue=*/{20}};

  // Stop all process to recovery
  neo_device_stop_scanning(out, error);

  // Setting motor running
  neo_device_set_motor_speed(out, 5, error);

  // device calibration
  neo_device_calibrate(out, error);

  // Stop motor
  // neo_device_set_motor_speed(out, 0, error);

  neo_device_stop_scanning(out, error);


  return out;
} catch ( const std::exception& e ) {
  *error = neo_error_construct(e.what());
  return nullptr;
}

void neo_device_destruct(neo_device_s device) {
  NEO_ASSERT(device);

  try {
    neo_error_s ignore = nullptr;
    neo_device_stop_scanning(device, &ignore);
  } catch (...) {
    // nothing we can do here
  }

  neo::serial::device_destruct(device->serial);
  delete device;
}

void neo_device_start_scanning(neo_device_s device, neo_error_s* error) try {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  if (device->is_scanning)
    return;

  neo::protocol::write_command(device->serial,
      neo::protocol::DATA_ACQUISITION_START);

  neo::protocol::read_response_header(device->serial,
      neo::protocol::DATA_ACQUISITION_START);

  device->scan_queue.clear();
  device->is_scanning = true;
  device->stop_thread = false;

  std::thread th = std::thread(neo_device_accumulate_scans, device);
  th.detach();
} catch (const std::exception& e) {
  *error = neo_error_construct(e.what());
}

void neo_device_stop_scanning(neo_device_s device, neo_error_s* error) try {
  NEO_ASSERT(device);
  NEO_ASSERT(error);

  if (!device->is_scanning)
    return;
  device->stop_thread = true;

  neo::protocol::write_command(device->serial,
      neo::protocol::DATA_ACQUISITION_STOP);

  // Wait some time for a few reasons: reference to sweep-sdk
  std::this_thread::sleep_for(std::chrono::milliseconds(35));

  try {
    neo::protocol::read_response_header(device->serial,
        neo::protocol::DATA_ACQUISITION_STOP);
  } catch ( const std::exception& ignore ) {
    (void) ignore;
  }

  // Flush any bytes left over
  neo::serial::device_flush(device->serial);

  neo::protocol::write_command(device->serial,
      neo::protocol::DATA_ACQUISITION_STOP);

  neo::protocol::read_response_header(device->serial,
      neo::protocol::DATA_ACQUISITION_STOP);

  device->is_scanning = false;
} catch ( const std::exception& e ) {
  *error = neo_error_construct(e.what());
}

neo_scan_s neo_device_get_scan(neo_device_s device, neo_error_s* error) try {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(device->is_scanning);

  auto out = device->scan_queue.dequeue();

  if ( out.error != nullptr ) {
    std::rethrow_exception(out.error);
  }

  return out.scan.release();
} catch ( const std::exception& e ) {
  *error = neo_error_construct(e.what());
  return nullptr;
}

int32_t neo_scan_get_number_of_samples(neo_scan_s scan) {
  NEO_ASSERT(scan);
  NEO_ASSERT(scan->count >= 0);

  return scan->count;
}

float neo_scan_get_angle(neo_scan_s scan, int32_t sample) {
  NEO_ASSERT(scan);
  NEO_ASSERT(sample >= 0 && sample < scan->count &&
      "sample index out of bounds.");

  return scan->samples[sample].angle;
}

int32_t neo_scan_get_distance(neo_scan_s scan, int32_t sample) {
  NEO_ASSERT(scan);
  NEO_ASSERT(sample >= 0 && sample < scan->count &&
      "sample index out of bounds.");

  return scan->samples[sample].distance;
}

/*
int32_t neo_scan_get_signal_strength(neo_scan_s scan, int32_t sample) {
  NEO_ASSERT(scan);
  NEO_ASSERT(sample >= 0 && sample < scan->count &&
      "sample index out of bounds.");

  return scan->samples[sample].signal_strength;
}
*/

void neo_scan_destruct(neo_scan_s scan) {
  NEO_ASSERT(scan);

  delete scan;
}

int32_t neo_device_get_motor_speed(neo_device_s device,
    neo_error_s* error) try {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  neo::protocol::write_command(device->serial, neo::protocol::MOTOR_INFORMATION);

  const auto response = neo::protocol::read_response_info_motor(device->serial);

  int32_t speed = neo::protocol::ascii_bytes_to_integral(response.motor_speed);
  NEO_ASSERT(speed >= 0);

  return speed;
} catch ( const std::exception& e ) {
  *error = neo_error_construct(e.what());
  return -1;
}

void neo_device_set_motor_speed(neo_device_s device, int32_t hz,
    neo_error_s* error) try {
  NEO_ASSERT(device);
  NEO_ASSERT(hz >= 0 && hz <= 10);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  uint8_t args[2] = {0};
  neo::protocol::integral_to_ascii_bytes(hz, args);

  neo::protocol::write_command_with_arguments(device->serial,
      neo::protocol::MOTOR_SPEED_ADJUST, args);
  neo::protocol::read_response_param(device->serial,
      neo::protocol::MOTOR_SPEED_ADJUST);

  if ( 0 != hz ) {
    printf("Wait the motor speed stabilizes...\n");
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

} catch ( const std::exception& e ) {
  *error = neo_error_construct(e.what());
}

void neo_device_reset(neo_device_s device, neo_error_s* error) try {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  neo::protocol::write_command(device->serial, neo::protocol::RESET_DEVICE);
} catch ( const std::exception& e ) {
  *error = neo_error_construct(e.what());
}

void neo_device_calibrate(neo_device_s device, neo_error_s* error) try {
  NEO_ASSERT(device);
  NEO_ASSERT(error);
  NEO_ASSERT(!device->is_scanning);

  neo::protocol::write_command(device->serial,
      neo::protocol::DEVICE_CALIBRATION);

  printf("wait device calibration...  \n");


  neo::protocol::read_response_header(device->serial,
      neo::protocol::DEVICE_CALIBRATION);
} catch ( const std::exception& e ) {
  *error = neo_error_construct(e.what());
}


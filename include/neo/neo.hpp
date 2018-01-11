#ifndef _NEO_HPP_
#define _NEO_HPP_

/*
 * C++ Wrapper around the low-level primitives.
 * Automatically handles resource management.
 *
 * neo::neo  - device to interact with
 * neo::scan   - a full scan returned by the device
 * neo::sample - a single sample in a full scan
 *
 * On error neo::device_error gets thrown.
 */

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include <neo/neo.h>

namespace neo {

// Error reporting

struct device_error final : std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

// Interface

struct sample {
  const float angle;
  const std::int32_t distance;
  //const std::int32_t signal_strength;
};

struct scan {
  std::vector<sample> samples;
};

class neo {
public:
  neo(const char* port);
  neo(const char* port, std::int32_t baudrate);

  void start_scanning();
  void stop_scanning();

  std::int32_t get_motor_speed();
  void set_motor_speed(std::int32_t speed);

  std::int32_t get_sample_rate();
  void set_sample_rate(std::int32_t speed);

  scan get_scan();

  void reset();

  void calibrate();

private:
  std::unique_ptr<::neo_device, decltype(&::neo_device_destruct)> device;
};

// Implementation

namespace detail {
struct error_to_exception {
  operator ::neo_error_s*() { return &error; }

  ~error_to_exception() noexcept(false) {
    if (error) {
      device_error e{::neo_error_message(error)};
      ::neo_error_destruct(error);
      throw e;
    }
  }

  ::neo_error_s error = nullptr;
};
}

neo::neo(const char* port)
    : device{::neo_device_construct_simple(port, detail::error_to_exception{}), &::neo_device_destruct} {}

neo::neo(const char* port, std::int32_t baudrate)
    : device{::neo_device_construct(port, baudrate, detail::error_to_exception{}), &::neo_device_destruct} {}

void neo::start_scanning() { ::neo_device_start_scanning(device.get(), detail::error_to_exception{}); }

void neo::stop_scanning() { ::neo_device_stop_scanning(device.get(), detail::error_to_exception{}); }

std::int32_t neo::get_motor_speed() { return ::neo_device_get_motor_speed(device.get(), detail::error_to_exception{}); }

void neo::set_motor_speed(std::int32_t speed) {
  ::neo_device_set_motor_speed(device.get(), speed, detail::error_to_exception{});
}

std::int32_t neo::get_sample_rate() { return ::neo_device_get_sample_rate(device.get(), detail::error_to_exception{}); }

void neo::set_sample_rate(std::int32_t rate) {
  ::neo_device_set_sample_rate(device.get(), rate, detail::error_to_exception{});
}

scan neo::get_scan() {
  using scan_owner = std::unique_ptr<::neo_scan, decltype(&::neo_scan_destruct)>;

  scan_owner releasing_scan{::neo_device_get_scan(device.get(), detail::error_to_exception{}), &::neo_scan_destruct};

  auto num_samples = ::neo_scan_get_number_of_samples(releasing_scan.get());

  scan result;
  result.samples.reserve(num_samples);

  for (std::int32_t n = 0; n < num_samples; ++n) {
    auto angle = ::neo_scan_get_angle(releasing_scan.get(), n);
    auto distance = ::neo_scan_get_distance(releasing_scan.get(), n);
    //auto signal = ::neo_scan_get_signal_strength(releasing_scan.get(), n);

    result.samples.push_back(sample{angle, distance});
  }

  return result;
}

void neo::reset() { ::neo_device_reset(device.get(), detail::error_to_exception{}); }
void neo::calibrate() { ::neo_device_calibrate(device.get(), detail::error_to_exception{}); }

} // namespace neo

#endif // _NEO_HPP_

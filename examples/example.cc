// Make use of the CMake build system or compile manually, e.g. with:
// g++ -std=c++11 example.cc -lneo

#include <cstdlib>
#include <iostream>

#include <neo/neo.hpp>

int main(int argc, char* argv[]) try {
  if (argc != 3) {
    std::cerr << "Usage: ./example-c++ device baudrate\n";
    return EXIT_FAILURE;
  }

  std::cout << "Constructing neo device..." << std::endl;
  neo::neo device{argv[1], atoi(argv[2])};

  std::cout << "Motor Speed Setting: " << device.get_motor_speed()
    << " Hz" << std::endl;
//  std::cout << "Sample Rate Setting: " << device.get_sample_rate()
//    << " Hz" << std::endl;

  std::cout << "Beginning data acquisition as soon as motor speed stabilizes..."
    << std::endl;
  device.start_scanning();

  for (auto n = 0; n < 10; ++n) {
    const neo::scan scan = device.get_scan();

    std::cout << "Scan #" << n << ":" << std::endl;
    for (const neo::sample& sample : scan.samples) {
      std::cout << "angle " << sample.angle << " distance "
        << sample.distance << std::endl;
        //<< " strength " << sample.signal_strength << "\n";
    }
  }

  device.stop_scanning();
} catch (const neo::device_error& e) {
  std::cerr << "Error: " << e.what() << std::endl;
}


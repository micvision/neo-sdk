Neo LiDAR SDK
---
[TOC]

### INSTALLATION

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
sudo cmake --build . --target install
sudo ldconfig
```

You can also using the script to install the SDK under ubuntu.

```bash
wget https://raw.githubusercontent.com/micvision/neo-sdk/neo-f1/script/installNeoSDK.sh
chmod +x installNeoSDK.sh
sudo ./installNeoSDK.sh
```

You can test with command:
```bash
# under the build/ directory
# ./example device baudrate
./example /dev/ttyUSB0 230400
```

For Windows users open a command prompt with administrative privileges:

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 14 2015 Win64"
cmake --build . --config Release
cmake --build . --target install --config Release
```

The above command assumes Visual Studio 2015. If you have a different version installed, change the value. ie:

    Visual Studio 11 2012 Win64 = Generates Visual Studio 11 (VS 2012) project files for x64 architecture
    Visual Studio 12 2013 Win64 = Generates Visual Studio 12 (VS 2013) project files for x64 architecture
    Visual Studio 14 2015 Win64 = Generates Visual Studio 14 (VS 2015) project files for x64 architecture
    Visual Studio 15 2017 Win64 = Generates Visual Studio 15 (VS 2017) project files for x64 architecture

Additionally, the above commands assume you want to build a x64 (64bit) verison of the library. To build a x86 (32 bit) version, simply drop the `Win64`. i.e.:

    Visual Studio 14 2015 = Generates Visual Studio 14 (VS 2015) project files for x86 architecture


Then be sure to add the installation directories for the library and the header files to the environment `PATH` variable. For the above installation that would be something like `C:\Program Files\neo\lib` for the library and `C:\Program Files\neo\inlcude\neo` for the headers. You may have to restart the computer before the changes take effect.

### Usage

- Include `<neo/neo.hpp>` for the C++ interface.
- Link `libneo.so` with `-lneo`.

If you used CMAKE build system, adding below to your `CMakeLists.txt`:

    FindPackage(Neo REQUIRED)
    target_link_libraries(.. ${LIBNEO_LIBRARY})
    target_include_directories(.. ${LIBNEO_INCLUDE_DIR})

### APIs (C++)

1. 
``` C++
int32_t neo_get_version(void);
```

Returns the neo sdk version.

2.
``` C++
static neo_error_s neo_error_construct(const char*);
void neo_error_destruct(neo_error_s*);
```

Construct and destruct of neo_error_s.

3.
``` C++
neo(const char* port);
neo(const char* port, int32_t baudrate);
```

Construct of neo device based on a serial device port (e.g. `/dev/ttyACM0` on Linux or `COM8` on Windows)
or a `baudrate` (default 115200).

4.
``` C++
void start_scanning(void);
void stop_scanning(void);
```

Neo device start/stop scanning api.

5.
``` C++
int32_t get_motor_speed(void);
void set_motor_speed(int32_t speed);
```

Neo device get/set motor speed [range: 0-10].

6.
``` C++
scan get_scan(void);
```

Neo device get the scan data.

7.
``` C++
void reset(void);
void calibrate(void);
```

Reset the neo device.

### Neopy

The `neopy` directory is the python package for neo device. Please refer the [README.md](numpy/README.md).


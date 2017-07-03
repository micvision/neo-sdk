Neo LiDAR SDK
---
[toc]

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
cd script
sudo ./installNeoSDK.sh
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

- Include `<neo/neo.h>` for the C interface or `<neo/neo.hpp>` for the C++ interface.
- Link `libneo.so` with `-lneo`.

If you used CMAKE build system, adding below to your `CMakeLists.txt`:

    FindPackage(Neo REQUIRED)
    target_link_libraries(.. ${LIBNEO_LIBRARY})
    target_include_directories(.. ${LIBNEO_INCLUDE_DIR})

### APIs

``` C++
int32_t neo_get_version(void);
```

Returns the neo sdk version.

``` C++
static neo_error_s neo_error_construct(const char*);
void neo_error_destruct(neo_error_s);
```

Construct and destruct of neo_error_s.

``` C++
neo(const char*, neo_error_s*);                                           // C++
neo(const char*, int32_t, neo_error_s*);                                  // C++

neo_device_s neo_device_construct_simple(const char*, neo_error_s*);      // C
neo_device_s neo_device_construct(const char*, int32_t, neo_error_s*);    // C
```

Construct of neo device based on a serial device port (e.g. `/dev/ttyACM0` on Linux or `COM8` on Windows)
or a `bitrate` (default 115200).

``` C++
void start_scanning(void);                                       // C++
void stop_scanning(void);                                        // C++

void neo_device_start_scanning(neo_device_s, neo_error_s*);      // C
void neo_device_stop_scanning(neo_device_s, neo_error_s*);       // C
```

Neo device start/stop scanning api.

``` C++
ini32_t get_motor_speed(void);                                          // C++
void set_motor_speed(int32_t speed);                                    // C++

int32_t neo_device_get_motor_speed(neo_device_s, neo_error_s*);         // C
void neo_device_set_motor_speed(neo_device_s, int32_t, neo_error_s*);   // C
```

Neo device get/set motor speed [range: 0-10].

``` C++
int32_t get_sample_rate(void);                                           // C++
void set_sample_rate(int32_t rate);                                      // C++

int32_t neo_device_get_sample_rate(neo_device_s, neo_error_s*);          // C
void neo_device_set_sample_rate(neo_device_s, int32_t, neo_error_s*);    // C
```

Neo device set/get the sample rate.

``` C++
scan get_scan(void);                                                    // C++

neo_scan_s neo_device_get_scan(neo_device_s, neo_error_s);              // C
int32_t neo_scan_get_number_of_samples(neo_scan_s);                     // C
int32_t neo_scan_get_angle(neo_scan_s, int32_t);                        // C
int32_t neo_scan_get_distance(neo_scan_s, int32_t);                     // C
int32_t neo_scan_get_signal_strength(neo_scan_s, int32_t);              // C
```

Neo device get the scan data.
Get the `number`/`angle`/`distance`/`signal strength` of scan.

``` C++
void reset(void);                                                       // C++

void neo_device_reset(neo_device_s, neo_error_s);                       // C
```

Reset the neo device.

### Neopy

The `neopy` directory is the python package for neo device. Please refer the [README.md](numpy/README.md).


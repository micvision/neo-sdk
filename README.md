# libneo

Neo LiDAR SDK.

### INSTALL

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
sudo cmake --build . --target install
sudo ldconfig
```


For Windows users open a command prompt with administrative access:

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


```bash
cmake ..  -DDUMMY=On -G "Visual Studio 14 2015 Win64"
```

Then be sure to add the installation directories for the library and the header files to the environment `PATH` variable. For the above installation that would be something like `C:\Program Files\neo\lib` for the library and `C:\Program Files\neo\inlcude\neo` for the headers. You may have to restart the computer before the changes take effect.

### Usage

- Include `<neo/neo.h>` for the C interface or `<neo/neo.hpp>` for the C++ interface.
- Link `libneo.so` with `-lneo`.

In your `CMakeLists.txt`:

    FindPackage(Neo REQUIRED)
    target_link_libraries(.. ${LIBNEO_LIBRARY})
    target_include_directories(.. ${LIBNEO_INCLUDE_DIR})



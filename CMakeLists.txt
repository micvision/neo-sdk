cmake_minimum_required(VERSION 2.8.11)
project(neo C CXX)


# Major for breaking ABI changes. Minor for features. Patch for bugfixes.

set(NEO_VERSION_MAJOR 0)
set(NEO_VERSION_MINOR 1)
set(NEO_VERSION_PATCH 0)


option(DUMMY "Build dummy libneo always returning static point cloud data. No device needed." OFF)


# Platform specific compiler and linker options.

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -fvisibility=hidden -fPIC -fno-rtti")
  set(libneo_OS unix)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -fvisibility=hidden -fPIC -fno-rtti")
  set(libneo_OS unix)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(libneo_OS win)

  # Turn on the ability to create folders to organize projects (.vcproj)
  # It creates "CMakePredefinedTargets" folder by default and adds CMake
  # defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj to that folder
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # define NEO_EXPORTS such that neo.h will export symbols marked by NEO_API
  add_definitions(-DNEO_EXPORTS)

else()
  message(FATAL_ERROR "System not yet supported. Please open a ticket.")

endif()


# Generate include/neo/config.h on the fly and insert version numbers.

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/neo/config.h)


# C++11 requires us to link in pthreads and equivalents.

find_package(Threads REQUIRED)


# libneo target.

file(GLOB libneo_OS_SOURCES src/${libneo_OS}/*.cpp)

if (DUMMY)
  set(libneo_IMPL_SOURCES src/dummy.cpp)
else()
  set(libneo_IMPL_SOURCES src/neo.cpp)
endif()

set(libneo_SOURCES ${libneo_OS_SOURCES} ${libneo_IMPL_SOURCES} src/protocol.cpp)
file(GLOB libneo_HEADERS include/*.h include/neo/*.h include/neo/*.hpp)

add_library(neo SHARED ${libneo_SOURCES} ${libneo_HEADERS})
target_include_directories(neo PRIVATE include include/neo ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(neo ${CMAKE_THREAD_LIBS_INIT})

set_property(TARGET neo PROPERTY VERSION "${NEO_VERSION_MAJOR}.${NEO_VERSION_MINOR}.${NEO_VERSION_PATCH}")
set_property(TARGET neo PROPERTY SOVERSION "${NEO_VERSION_MAJOR}")

install(TARGETS neo DESTINATION lib)
install(DIRECTORY include/neo DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/neo/config.h DESTINATION include/neo)

add_executable(example examples/example.cpp)
target_include_directories(example PRIVATE include include/neo ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(example neo ${CMAKE_THREAD_LIBS_INIT})


# Make FindPackage(Neo) work for CMake users.
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/NeoConfig.cmake DESTINATION lib/cmake/neo)


# make uninstall target.

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeUninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/cmake/CMakeUninstall.cmake"
               IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/CMakeUninstall.cmake)

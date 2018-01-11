#ifndef _NEO_H_
#define _NEO_H_

#include <stdbool.h>
#include <stdint.h>

#include <neo/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__ || defined __MINGW32__
// If we are building a dll, set NEO_API to export symbols
#ifdef NEO_EXPORTS
#ifdef __GNUC__
#define NEO_API __attribute__((dllexport))
#else
#define NEO_API __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define NEO_API __attribute__((dllimport))
#else
#define NEO_API __declspec(dllimport)
#endif
#endif
#else
#if __GNUC__ >= 4
#define NEO_API __attribute__((visibility("default")))
#else
#define NEO_API
#endif
#endif

#ifndef NEO_ASSERT
#include <assert.h>
#define NEO_ASSERT(x) assert(x)
#endif

NEO_API int32_t neo_get_version(void);
NEO_API bool neo_is_abi_compatible(void);

typedef struct neo_error* neo_error_s;
typedef struct neo_device* neo_device_s;
typedef struct neo_scan* neo_scan_s;

NEO_API const char* neo_error_message(neo_error_s error);
NEO_API void neo_error_destruct(neo_error_s error);

NEO_API neo_device_s neo_device_construct_simple(const char* port, neo_error_s* error);
NEO_API neo_device_s neo_device_construct(const char* port, int32_t baudrate, neo_error_s* error);
NEO_API void neo_device_destruct(neo_device_s device);

NEO_API void neo_device_start_scanning(neo_device_s device, neo_error_s* error);
NEO_API void neo_device_stop_scanning(neo_device_s device, neo_error_s* error);

NEO_API neo_scan_s neo_device_get_scan(neo_device_s device, neo_error_s* error);
NEO_API void neo_scan_destruct(neo_scan_s scan);

NEO_API int32_t neo_scan_get_number_of_samples(neo_scan_s scan);
NEO_API float neo_scan_get_angle(neo_scan_s scan, int32_t sample);
NEO_API int32_t neo_scan_get_distance(neo_scan_s scan, int32_t sample);
NEO_API int32_t neo_scan_get_signal_strength(neo_scan_s scan, int32_t sample);

NEO_API int32_t neo_device_get_motor_speed(neo_device_s device, neo_error_s* error);
NEO_API void neo_device_set_motor_speed(neo_device_s device, int32_t hz, neo_error_s* error);

NEO_API int32_t neo_device_get_sample_rate(neo_device_s device, neo_error_s* error);
NEO_API void neo_device_set_sample_rate(neo_device_s device, int32_t hz, neo_error_s* error);

NEO_API void neo_device_reset(neo_device_s device, neo_error_s* error);

NEO_API void neo_device_calibrate(neo_device_s device, neo_error_s* error);

#ifdef __cplusplus
}
#endif

#endif // _NEO_H_

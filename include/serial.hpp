#ifndef _SERIAL_HPP_
#define _SERIAL_HPP_

/*
 * Communication with serial devices.
 * Implementation detail; not exported.
 */

#include "error.hpp"
#include "neo.h"

#include <stdint.h>

namespace neo {
namespace serial {

// typedef struct device* device_s;
using device_s = struct device*;

struct error : neo::error::error {
  using base = neo::error::error;
  using base::base;
};

device_s device_construct(const char* port, int32_t baudrate);
void device_destruct(device_s serial);

void device_read(device_s serial, void* to, int32_t len);
void device_write(device_s serial, const void* from, int32_t len);
void device_flush(device_s serial);

}  // namespace serial
}  // namespace neo

#endif  // _SERIAL_HPP_

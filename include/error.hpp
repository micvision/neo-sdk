#ifndef _ERROR_HPP_
#define _ERROR_HPP_

/*
 * Internal base error sub-system errors inherit from.
 * Implementation detail; not exported.
 */

#include <stdexcept>

namespace neo {
namespace error {

struct error : std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

}  // namespace error
}  // namespace neo

#endif  // end _ERROR_HPP_

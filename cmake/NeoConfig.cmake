# Exports:
#  LIBNEO_FOUND
#  LIBNEO_INCLUDE_DIR
#  LIBNEO_LIBRARY
# Hints:
#  LIBNEO_LIBRARY_DIR

find_path(LIBNEO_INCLUDE_DIR
          neo/neo.h neo/neo.hpp)

find_library(LIBNEO_LIBRARY
             NAMES neo
             HINTS "${LIBNEO_LIBRARY_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Neo DEFAULT_MSG LIBNEO_LIBRARY LIBNEO_INCLUDE_DIR)
mark_as_advanced(LIBNEO_INCLUDE_DIR LIBNEO_LIBRARY)

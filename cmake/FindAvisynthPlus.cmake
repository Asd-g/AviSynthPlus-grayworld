# FindAvisynthPlus.cmake - Finds the Avisynth+ headers
#
# This module finds the correct include directory so that C++ code can
# consistently use `#include <avisynth.h>`.
#
# It respects the standard CMAKE_PREFIX_PATH variable.
# To specify a custom SDK location, run cmake with:
#   -D CMAKE_PREFIX_PATH="C:/path/to/your/sdk"
#
# As a fallback, it also respects the AVISYNTHPLUS_ROOT_DIR variable.
#
# It defines the following imported target:
#   AvisynthPlus::headers  - An INTERFACE target providing the include directory.

include(FindPackageHandleStandardArgs)

# 1. Find the directory that DIRECTLY contains avisynth.h or avisynth_c.h
#    We use PATH_SUFFIXES to check common layouts.
find_path(AvisynthPlus_INCLUDE_DIR
  NAMES
    avisynth.h
    avisynth_c.h
  HINTS
    # Allow a specific root directory to be provided as a hint
    ENV AVISYNTHPLUS_ROOT
    ${AVISYNTHPLUS_ROOT_DIR}
  PATH_SUFFIXES
    # For namespaced layouts like /usr/include/avisynth/avisynth.h
    include/avisynth
    sdk/include/avisynth
    # For flat layouts like C:/sdk/include/avisynth.h
    include
    sdk/include
)

# 2. Handle the standard arguments for find_package
find_package_handle_standard_args(AvisynthPlus
  FOUND_VAR AvisynthPlus_FOUND
  REQUIRED_VARS
    AvisynthPlus_INCLUDE_DIR
  FAIL_MESSAGE
    "Could not find Avisynth+ headers (avisynth.h or avisynth_c.h). Please specify the SDK location by setting CMAKE_PREFIX_PATH or AVISYNTHPLUS_ROOT_DIR."
)

# 3. If found, create the INTERFACE imported target
if(AvisynthPlus_FOUND)
  if(NOT TARGET AvisynthPlus::headers)
    # If the target doesn't exist at all, create the real IMPORTED target.
    add_library(AvisynthPlus::headers INTERFACE IMPORTED)
    set_target_properties(AvisynthPlus::headers PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${AvisynthPlus_INCLUDE_DIR}"
    )
  else()
    # If the target DOES exist (it's our placeholder), it is NOT imported.
    target_include_directories(AvisynthPlus::headers INTERFACE "${AvisynthPlus_INCLUDE_DIR}")
  endif()
endif()

# Mark the internal variable as advanced
mark_as_advanced(AvisynthPlus_INCLUDE_DIR)

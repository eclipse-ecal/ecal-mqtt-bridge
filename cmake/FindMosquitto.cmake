include(ExternalProject)
include(FindPackageHandleStandardArgs)

if (NOT MOSQUITTO_INCLUDE_DIR)
  find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h mosquittopp.h)
endif()

if (NOT MOSQUITTO_LIBRARY)
  find_library(
    MOSQUITTO_LIBRARY
    NAMES mosquitto mosquittod
    PATH_SUFFIXES lib)
endif()

if (NOT MOSQUITTO_LIBRARY_CPP)
  find_library(
    MOSQUITTO_LIBRARY_CPP
    NAMES mosquittopp mosquittoppd
    PATH_SUFFIXES lib)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  Mosquitto DEFAULT_MSG
  MOSQUITTO_LIBRARY MOSQUITTO_LIBRARY_CPP MOSQUITTO_INCLUDE_DIR)

message(STATUS "libmosquitto include dir: ${MOSQUITTO_INCLUDE_DIR}")
message(STATUS "libmosquitto: ${MOSQUITTO_LIBRARY}")
message(STATUS "libmosquittopp: ${MOSQUITTO_LIBRARY_CPP}")
set(MOSQUITTO_LIBRARIES ${MOSQUITTO_LIBRARY};${MOSQUITTO_LIBRARY_CPP})

mark_as_advanced(MOSQUITTO_INCLUDE_DIR MOSQUITTO_LIBRARIES)


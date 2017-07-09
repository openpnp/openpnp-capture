#
# CMAKE build file for OpenPnP Capture library
#
# This generates make files for several build systems,
# such as GNU Make, Ninja and visual studio
#
# When invoking on Windows systems, make sure the
# compiler (Visual Studio) is in the search path.
#
# Author: Niels A. Moseley
#

cmake_minimum_required(VERSION 3.0)
project (openpnp-capture)

# add include directory 
include_directories(include)

IF (WIN32)

# add files for WIN32
set (SOURCE win/libmain.cpp
            win/context.cpp
            win/logging.cpp
            win/stream.cpp)

# add windows-specific test application
add_subdirectory(win/tests)

ELSEIF(APPLE)
# TODO: add files for APPLE / OSX
  message( FATAL_ERROR "Support for APPLE systems is pending." )
ELSEIF(UNIX)
# TODO: add files for UNIX/LINUX
  message( FATAL_ERROR "Support for UNIX systems is pending." )
ENDIF()

add_library(openpnp-capture SHARED ${SOURCE})

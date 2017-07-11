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

cmake_minimum_required(VERSION 3.1)
project (openpnp-capture)

# force C++11 standard
set(CMAKE_CXX_STANDARD 11)

# add include directory 
include_directories(include)

IF (WIN32)

# add files for WIN32
set (SOURCE common/libmain.cpp
            common/context.cpp
            common/logging.cpp
            common/stream.cpp
            win/platformcontext.cpp
            win/platformstream.cpp)

# add windows-specific test application
add_subdirectory(win/tests)

ELSEIF(APPLE)
set (SOURCE common/libmain.cpp
            common/context.cpp
            common/logging.cpp
            common/stream.cpp)
            
ELSEIF(UNIX)
# TODO: add files for UNIX/LINUX
  message( FATAL_ERROR "Support for UNIX systems is pending." )
ENDIF()

add_library(openpnp-capture SHARED ${SOURCE})

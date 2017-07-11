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
project (openpnp-capture-test)

# add include directory 
include_directories(../include ..)

set (SOURCE main.cpp ../../common/logging.cpp)

add_executable(openpnp-capture-test ${SOURCE})

target_link_libraries(openpnp-capture-test openpnp-capture)
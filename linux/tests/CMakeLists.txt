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

set (SOURCE main.cpp ../../common/logging.cpp)

# add include directory 
include_directories(../include ..)

add_executable(openpnp-capture-test ${SOURCE})

target_link_libraries(openpnp-capture-test openpnp-capture)
target_link_libraries(openpnp-capture-test ${TurboJPEG_LIBRARIES})

########################################################
### GTK test application
########################################################

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
add_definitions(${GTK3_CFLAGS_OTHER})

set (SOURCE2 gtkmain.cpp ../../common/logging.cpp)

add_executable(oc-gtk ${SOURCE2})

target_link_libraries(oc-gtk openpnp-capture)
target_link_libraries(oc-gtk ${TurboJPEG_LIBRARIES})
target_link_libraries(oc-gtk ${GTK3_LIBRARIES})

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

    # create the library
    add_library(openpnp-capture SHARED ${SOURCE})

    # add windows-specific test application
    add_subdirectory(win/tests)

ELSEIF(APPLE)
    set (SOURCE common/libmain.cpp
                common/context.cpp
                common/logging.cpp
                common/stream.cpp
                mac/platformcontext.mm
                mac/platformstream.mm)


    # create the library
    add_library(openpnp-capture SHARED ${SOURCE})

    # include OS X specific frameworks
    target_link_libraries(openpnp-capture
        "-framework AVFoundation"
        "-framework Foundation"
        "-framework CoreMedia"
        )

    # add mac specific test application
    add_subdirectory(mac/tests)

ELSEIF(UNIX)
    
    set (SOURCE common/libmain.cpp
                common/context.cpp
                common/logging.cpp
                common/stream.cpp
                linux/platformcontext.cpp
                linux/platformstream.cpp
                linux/mjpeghelper.cpp)

    # need pthreads for std::thread
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    add_library(openpnp-capture SHARED ${SOURCE})
    target_link_libraries(openpnp-capture Threads::Threads)
    target_link_libraries(openpnp-capture turbojpeg)
    
    # add linux-specific test application
    add_subdirectory(linux/tests)

ENDIF()


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


# library version
add_definitions(-D__LIBVER__="v0.01")

# determine number of bits of compiler
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set( COMPILERBITS "64 bit")
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set( COMPILERBITS "32 bit")
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

# check the build type and set the build type string
if(CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-D__BUILDTYPE__="release")
else(CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-D__BUILDTYPE__="debug")
endif(CMAKE_BUILD_TYPE MATCHES Release)


# add include directory 
include_directories(include)

IF (WIN32)
    # set the platform identification string
    add_definitions(-D__PLATFORM__="Win ${COMPILERBITS}")
    
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
    # set the platform identification string
    add_definitions(-D__PLATFORM__="OSX ${COMPILERBITS}")

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
        "-framework CoreVideo"
        "-framework Accelerate"
        )

    # add mac specific test application
    add_subdirectory(mac/tests)

ELSEIF(UNIX)
    # set the platform identification string
    add_definitions(-D__PLATFORM__="Linux ${COMPILERBITS}")

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

    # this will force using the static lib, which will will eventually want to 
    # do for all dependencies so we don't have to distribute them with our .so
    # find_library(TURBOJPEG_LIB NAMES turbojpeg.a turbojpeg)

    # but libturbojpeg is not compiled with -fPIC which means it can't be static
    # linked into a shared lib. So instead we will eventually have to build it
    # ourselves with -fPIC and then we can use this to specify the location:
    # ADD_LIBRARY(turbojpeg STATIC IMPORTED)
    # SET_TARGET_PROPERTIES(turbojpeg PROPERTIES IMPORTED_LOCATION ../libjpegturbo/whatever/libjpegturbo.a)

    # this lets us specify the full path of the turbojpeg library. this is required
    # because of a bug in the library distribution that does not create the correct
    # symlink. See https://github.com/OpenKinect/libfreenect2/issues/36
    # Note: We should be able to specify this on the command line, as well, so we don't have
    # to hardcode the path here.
    ADD_LIBRARY(turbojpeg SHARED IMPORTED)
    SET_TARGET_PROPERTIES(turbojpeg PROPERTIES IMPORTED_LOCATION /usr/lib/x86_64-linux-gnu/libturbojpeg.so.0.0.0)

    target_link_libraries(openpnp-capture turbojpeg)
    
    # add linux-specific test application
    add_subdirectory(linux/tests)

ENDIF()


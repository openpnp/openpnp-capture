#
# CMAKE build file for OpenPnP Capture library
#
# This generates make files for several build systems,
# such as GNU Make, Ninja and visual studio
#
# When invoking on Windows systems, make sure the
# compiler (Visual Studio) is in the search path.
#
# Author: Niels A. Moseley, Jason von Nieda
#

cmake_minimum_required(VERSION 3.1)
project (openpnp-capture)

set(POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# make CMAKE search the current cmake dir inside the
# current project
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# force C++11 standard
set(CMAKE_CXX_STANDARD 11)

# load module that gets info from GIT 
# see: http://brianmilco.blogspot.nl/2012/11/cmake-automatically-use-git-tags-as.html
include(GetGitRevisionDescription)

# create library version from GIT tag using cmake/version.h.in as a template
# and write it to common/version.h
git_describe(GITVERSION --tags)
MESSAGE(STATUS "Using GIT tag: " ${GITVERSION} )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/common/version.h)

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
    
    # remove annoying 'unsafe' function warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

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
                mac/platformstream.mm
                mac/uvcctrl.mm)


    # create the library
    add_library(openpnp-capture SHARED ${SOURCE})

    # include OS X specific frameworks
    target_link_libraries(openpnp-capture
        "-framework AVFoundation"
        "-framework Foundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework Accelerate"
        "-framework IOKit"
        )

    # add mac specific test application
    add_subdirectory(mac/tests)

ELSEIF(UNIX)

    set(ENABLE_SHARED OFF)
    add_subdirectory(linux/contrib/libjpeg-turbo-dev)

    # Compile add libjpeg-turbo using Autotools
    # as CMAKE is not supported on linux :-/
    #
    # Note that will do not install the project 
    # but statically link with it later..
    #

    #include(ExternalProject)
    #ExternalProject_Add(turbojpeg
    #    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/linux/contrib/libjpeg-turbo-1.5.2
    #    CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/linux/contrib/libjpeg-turbo-1.5.2/configure
    #    BUILD_COMMAND ${MAKE}
    #    INSTALL_COMMAND "")

    # set the platform identification string
    add_definitions(-D__PLATFORM__="Linux ${COMPILERBITS}")

    set (SOURCE common/libmain.cpp
                common/context.cpp
                common/logging.cpp
                common/stream.cpp
                linux/platformcontext.cpp
                linux/platformstream.cpp
                linux/mjpeghelper.cpp
                linux/yuvconverters.cpp)

    # need pthreads for std::thread
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    add_library(openpnp-capture SHARED ${SOURCE})
    target_link_libraries(openpnp-capture Threads::Threads)
    target_link_libraries(openpnp-capture turbojpeg-static)


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
    
    #ADD_LIBRARY(turbojpeg SHARED IMPORTED)
    #if(EXISTS "/usr/lib/x86_64-linux-gnu/libturbojpeg.so.0.1.0")
    #    SET_TARGET_PROPERTIES(turbojpeg PROPERTIES IMPORTED_LOCATION /usr/lib/x86_64-linux-gnu/libturbojpeg.so.0.1.0)
    #else()
    #    SET_TARGET_PROPERTIES(turbojpeg PROPERTIES IMPORTED_LOCATION /usr/lib/x86_64-linux-gnu/libturbojpeg.so.0.0.0)
    #endif()
    
    #target_link_libraries(openpnp-capture turbojpeg)
    
    # add linux-specific test application
    add_subdirectory(linux/tests)

ENDIF()


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
set(OPENPNP_CAPTURE_LIB_VERSION "0.0.23" CACHE STRING "openpnp-capture library version")
set(OPENPNP_CAPTURE_LIB_SOVERSION "0.0.23" CACHE STRING "openpnp-capture library soversion")

# make sure the libjpegturbo is compiled with the
# position independent flag -fPIC
IF (UNIX)
    set(POSITION_INDEPENDENT_CODE TRUE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF()

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

# create our capture library
add_library(openpnp-capture SHARED common/libmain.cpp
                                   common/context.cpp
                                   common/logging.cpp
                                   common/stream.cpp)

# define common properties
set_target_properties(openpnp-capture PROPERTIES
                      VERSION ${OPENPNP_CAPTURE_LIB_VERSION}
                      SOVERSION ${OPENPNP_CAPTURE_LIB_SOVERSION})

IF (WIN32)
    if(MSVC)
        # build with static runtime rather than DLL based so that we
        # don't have to distribute it
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    endif()

    # set the platform identification string
    add_definitions(-D__PLATFORM__="Win ${COMPILERBITS}")
    
    # remove annoying 'unsafe' function warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    # add files for WIN32
    target_sources(openpnp-capture PRIVATE win/platformcontext.cpp
                                           win/platformstream.cpp)
    target_link_libraries(openpnp-capture strmiids)

    # add windows-specific test application
    add_subdirectory(win/tests)

ELSEIF(APPLE)
    # set the platform identification string
    add_definitions(-D__PLATFORM__="OSX ${COMPILERBITS}")

    target_sources(openpnp-capture PRIVATE mac/platformcontext.mm
                                           mac/platformstream.mm
                                           mac/uvcctrl.mm)

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
    # install path resolving
    include(GNUInstallDirs)

    # set the platform identification string
    add_definitions(-D__PLATFORM__="Linux ${COMPILERBITS}")

    target_sources(openpnp-capture PRIVATE linux/platformcontext.cpp
                                           linux/platformstream.cpp
                                           linux/mjpeghelper.cpp
                                           linux/yuvconverters.cpp)

    # force include directories for libjpeg-turbo
    include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/linux/contrib/libjpeg-turbo-dev")

    # add pthreads library 
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)    
    target_link_libraries(openpnp-capture PUBLIC Threads::Threads)

    # add turbojpeg library
    find_package(PkgConfig REQUIRED)
    pkg_search_module(TurboJPEG libturbojpeg)
    if( TurboJPEG_FOUND )
        link_directories(${TurboJPEG_LIBDIR})
        target_include_directories(openpnp-capture PUBLIC ${TurboJPEG_INCLUDE_DIRS})
        target_link_libraries(openpnp-capture PUBLIC ${TurboJPEG_LIBRARIES})
    else()
        # compile libjpeg-turbo for MJPEG decoding support
        # right now, we need to disable SIMD because it
        # causes a compile problem.. we need to fix this
        # later...
        set(ENABLE_SHARED OFF)
        set(WITH_SIMD OFF)    
        set(TurboJPEG_LIBRARIES turbojpeg-static)  
        add_subdirectory(linux/contrib/libjpeg-turbo-dev)
        target_link_libraries(openpnp-capture PRIVATE ${TurboJPEG_LIBRARIES})
    endif()

    # add linux-specific test application
    add_subdirectory(linux/tests)

    # install lib and headers
    install(FILES include/openpnp-capture.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT headers)
    install(TARGETS openpnp-capture EXPORT openpnp-capture
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT libraries)

    # add cmake install target
    install(EXPORT openpnp-capture
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/openpnp-capture
            COMPONENT libraries)

ENDIF()


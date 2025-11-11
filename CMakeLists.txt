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

cmake_minimum_required(VERSION 3.10)
project (openpnp-capture)
set(OPENPNP_CAPTURE_LIB_VERSION "0.0.28" CACHE STRING "openpnp-capture library version")
set(OPENPNP_CAPTURE_LIB_SOVERSION "0" CACHE STRING "openpnp-capture library soversion")

# Library type options
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_STATIC_LIBS "Build static library" OFF)
option(LINK_DYNAMIC_MSVC_RUNTIME "Link MSVC runtime statically" OFF)

# Validate library type options
if(BUILD_STATIC_LIBS AND BUILD_SHARED_LIBS)
    message(WARNING "Both BUILD_STATIC_LIBS and BUILD_SHARED_LIBS are enabled. Building static library only.")
    set(BUILD_SHARED_LIBS OFF)
elseif(NOT BUILD_STATIC_LIBS AND NOT BUILD_SHARED_LIBS)
    message(STATUS "Neither BUILD_STATIC_LIBS nor BUILD_SHARED_LIBS specified. Defaulting to shared library.")
    set(BUILD_SHARED_LIBS ON)
endif()

# make sure the libjpegturbo is compiled with the
# position independent flag -fPIC
IF (UNIX)
    set(POSITION_INDEPENDENT_CODE TRUE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

    # Add O3 optimizations and native architecture for release builds
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
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

# create our capture library
if(BUILD_STATIC_LIBS AND NOT BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE STATIC)
    add_definitions(-DOPENPNPCAPTURE_STATIC)
    message(STATUS "Building static library")
else()
    set(LIBRARY_TYPE SHARED)
    message(STATUS "Building shared library")
endif()

add_library(openpnp-capture ${LIBRARY_TYPE} common/libmain.cpp
                                           common/context.cpp
                                           common/logging.cpp
                                           common/stream.cpp)

target_include_directories(openpnp-capture PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
# define common properties
set_target_properties(openpnp-capture PROPERTIES
                      VERSION ${OPENPNP_CAPTURE_LIB_VERSION}
                      SOVERSION ${OPENPNP_CAPTURE_LIB_SOVERSION})

IF (WIN32)
    if(MSVC)
        if(LINK_DYNAMIC_MSVC_RUNTIME)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
        else()
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        endif()
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
    include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/linux/contrib/libjpeg-turbo-3.1.2")

    # add pthreads library 
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)    
    target_link_libraries(openpnp-capture PRIVATE Threads::Threads)

    # add turbojpeg library
    find_package(PkgConfig REQUIRED)
    pkg_search_module(TurboJPEG libturbojpeg)
    if( TurboJPEG_FOUND )
        target_link_directories(openpnp-capture PRIVATE ${TurboJPEG_LIBDIR})
        target_include_directories(openpnp-capture PRIVATE ${TurboJPEG_INCLUDE_DIRS})
        target_link_libraries(openpnp-capture PRIVATE ${TurboJPEG_LIBRARIES})
    else()
        # compile libjpeg-turbo for MJPEG decoding support using ExternalProject
        include(ExternalProject)

        set(LIBJPEG_TURBO_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo)
        set(LIBJPEG_TURBO_INSTALL_DIR ${LIBJPEG_TURBO_PREFIX}/install)

        ExternalProject_Add(libjpeg-turbo-external
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/linux/contrib/libjpeg-turbo-3.1.2
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DENABLE_SHARED=OFF
                -DWITH_SIMD=ON
                -DWITH_TOOLS=OFF
                -DWITH_JAVA=OFF
                -DCMAKE_C_FLAGS=-fPIC
                -DCMAKE_CXX_FLAGS=-fPIC
                -DCMAKE_C_FLAGS_RELEASE=-O3 
                -DCMAKE_CXX_FLAGS_RELEASE=-O3
            BUILD_BYPRODUCTS 
                ${LIBJPEG_TURBO_INSTALL_DIR}/lib/libturbojpeg.a
        )

        # Create imported target for turbojpeg
        add_library(turbojpeg-static STATIC IMPORTED)
        set_target_properties(turbojpeg-static PROPERTIES
            IMPORTED_LOCATION ${LIBJPEG_TURBO_INSTALL_DIR}/lib/libturbojpeg.a
        )

        # Set up include directories
        target_include_directories(openpnp-capture PRIVATE ${LIBJPEG_TURBO_INSTALL_DIR}/include)

        # Make openpnp-capture depend on the external project
        add_dependencies(openpnp-capture libjpeg-turbo-external)
        add_dependencies(turbojpeg-static libjpeg-turbo-external)

        target_link_libraries(openpnp-capture PRIVATE turbojpeg-static)
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


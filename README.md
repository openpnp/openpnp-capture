![OpenPNP Logo](doc/logo_small.png)

OpenPnP Capture is a cross platform video capture library with a focus on machine vision. It's goals are:

* Native camera access on Windows, Linux and Mac. Implemented with DirectShow, V4L2 and AVFoundation respectively.
* Named device enumeration.
* Strong, repeatable unique IDs.
* Format control with support for at least YUV and MJPEG. MJPEG will allow the use of multiple USB cameras on a single hub.
* Auto and manual exposure control.
* Auto and manual focus control.
* Simple, common C wrapper for the listed APIs.

# Status

### Windows

| Feature       | Implemented   |
| ------------- |:-------------:|
| Device Enumeration | Yes |
| Capturing | Yes |
| MJPEG formats | Yes |
| YUV formats | Yes |
| Exposure control | Yes |
| Focus control | Yes / Untested |
| Zoom control | Yes |
| Framerate control | No |

### Linux

| Feature       | Implemented   |
| ------------- |:-------------:|
| Device Enumeration | Yes |
| Capturing | Yes |
| MJPEG formats | Yes |
| YUV formats | Yes |
| Exposure control | Yes / Untested |
| Focus control | Yes / Untested |
| Zoom control | Yes / Untested |
| Framerate control | No |

### OSX

| Feature       | Implemented   |
| ------------- |:-------------:|
| Device Enumeration | Yes |
| Capturing | Yes |
| MJPEG formats | Yes (dmb1) |
| YUV formats | Yes |
| Exposure control | No |
| Focus control | No |
| Zoom control | No |
| Framerate control | No |

# Building OpenPnP Capture

### Dependencies
* CMAKE 3.1 or better
* MAKE (osx, linux)
* Visual Studio 2013 + NMake or Ninja Build (windows)
* libjpeg-turbo (linux) -- will be merged into the project at a later time

### Build instructions (Windows)
Run the batch file 'bootstrap.bat' and choose the desired build system (VisualStudio/nmake or Ninja). Make sure the compiler (Visual Studio) is in the search path. 

Go to the build directory and run nmake or ninja to build the library and the test application.

### Build instructions (OSX)
TODO

### Build instructions (Linux)
Run 'bootstrap_linux.sh'. Run make.

## Supporting other platforms
* Implement all PlatformXXX classes, like in the win or linux directories.
* PlatformContext handles device and internal frame buffer format enumeration.
* PlatformStream is responsible for capturing and decoding the camera stream to a 8-bit per channel RGB frame buffer.
* Statically link external dependencies.

# Downloading Binaries

openpnp-capture is not ready for release, but you can download test binaries that are built automatically.

## Windows
1. Go to https://ci.appveyor.com/project/vonnieda/openpnp-capture
2. Click the Release or Debug build.
3. Select Artifacts.
4. Download the individual listed files.

## Linux
* https://s3-us-west-2.amazonaws.com/openpnp-capture/master/linux/libopenpnp-capture.so
* https://s3-us-west-2.amazonaws.com/openpnp-capture/master/linux/openpnp-capture-test

## OSX
* https://s3-us-west-2.amazonaws.com/openpnp-capture/master/mac/libopenpnp-capture.dylib
* https://s3-us-west-2.amazonaws.com/openpnp-capture/master/mac/openpnp-capture-test



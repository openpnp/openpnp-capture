![OpenPNP Logo](doc/logo_small.png)

OpenPnP Capture is a cross platform video capture library with a focus on machine vision. It's goals are:

* Native camera access on Windows, Linux and Mac. Implemented with DirectShow, V4L2 and AVFoundation respectively.
* Named device enumeration.
* Strong, repeatable unique IDs.
* Format control with support for at least YUV and MJPEG. MJPEG will allow the use of multiple USB cameras on a single hub.
* Auto and manual exposure control.
* Auto and manual focus control.
* Simple, common C wrapper for the listed APIs.

### Status

Working in the windows version:
* device enumeration.
* capturing from a device (using default frame size and format).
* partial c interface.
* exposure control.

Not working in the windows version:
* setting of frame size.
* setting of video format, e.g. MJPG etc.
* supported format enumeration.
* focus control.

## Buildin OpenPnP Capture

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

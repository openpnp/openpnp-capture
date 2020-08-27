/*

    OpenPnp-Capture: a video capture subsystem.

    Copyright (c) 2017 Jason von Nieda, Niels Moseley.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

/*!
*  @file
*  @brief C API for OpenPnP Capture Library
*/

#ifndef openpnp_capture_h
#define openpnp_capture_h

#include <stdint.h>

// 
//
#if defined(__clang__)
    #define SO_IMPORT 
    #define SO_EXPORT    
#elif defined(__GNUC__) || defined(__GNUG__)
    #define SO_IMPORT
    #define SO_EXPORT
#elif defined(_MSC_VER)
    #define SO_IMPORT __declspec(dllimport)
    #define SO_EXPORT __declspec(dllexport)
#else
    #error("Unknown compiler")
#endif

// make sure its exported/imported as pure C 
// even if we're compiling with a C++ compiler
#ifdef BUILD_OPENPNP_LIBRARY
    #ifdef __cplusplus
        #define DLLPUBLIC extern "C" SO_EXPORT
    #else
        #define DLLPUBLIC SO_EXPORT
    #endif
#else
    #ifdef __cplusplus
        #define DLLPUBLIC extern "C" SO_IMPORT
    #else
        #define DLLPUBLIC SO_IMPORT
    #endif
#endif

typedef void*    CapContext;    ///< an opaque pointer to the internal Context*
typedef int32_t  CapStream;     ///< a stream identifier (normally >=0, <0 for error)
typedef uint32_t CapResult;     ///< result defined by CAPRESULT_xxx
typedef uint32_t CapDeviceID;   ///< unique device ID
typedef uint32_t CapFormatID;   ///< format identifier 0 .. numFormats

// supported properties:
#define CAPPROPID_EXPOSURE      1
#define CAPPROPID_FOCUS         2 
#define CAPPROPID_ZOOM          3
#define CAPPROPID_WHITEBALANCE  4
#define CAPPROPID_GAIN          5
#define CAPPROPID_BRIGHTNESS    6
#define CAPPROPID_CONTRAST      7
#define CAPPROPID_SATURATION    8
#define CAPPROPID_GAMMA         9
#define CAPPROPID_HUE           10
#define CAPPROPID_SHARPNESS     11
#define CAPPROPID_BACKLIGHTCOMP 12
#define CAPPROPID_POWERLINEFREQ 13
#define CAPPROPID_LAST          14

typedef uint32_t CapPropertyID; ///< property ID (exposure, zoom, focus etc.)

typedef struct
{
    uint32_t width;     ///< width in pixels
    uint32_t height;    ///< height in pixels
    uint32_t fourcc;    ///< fourcc code (platform dependent)
    uint32_t fps;       ///< frames per second
    uint32_t bpp;       ///< bits per pixel
} CapFormatInfo;

#define CAPRESULT_OK  0
#define CAPRESULT_ERR 1
#define CAPRESULT_DEVICENOTFOUND 2
#define CAPRESULT_FORMATNOTSUPPORTED 3
#define CAPRESULT_PROPERTYNOTSUPPORTED 4

/********************************************************************************** 
     CONTEXT CREATION AND DEVICE ENUMERATION
**********************************************************************************/

/** Initialize the capture library
    @return The context ID.
*/
DLLPUBLIC CapContext Cap_createContext(void);

/** Un-initialize the capture library context
    @param ctx The ID of the context to destroy.
    @return The context ID.
*/
DLLPUBLIC CapResult Cap_releaseContext(CapContext ctx);

/** Get the number of capture devices on the system.
    note: this can change dynamically due to the
    pluggin and unplugging of USB devices.
    @param ctx The ID of the context.
    @return The number of capture devices found.
*/
DLLPUBLIC uint32_t Cap_getDeviceCount(CapContext ctx);

/** Get the name of a capture device.
    This name is meant to be displayed in GUI applications,
    i.e. its human readable.

    if a device with the given index does not exist,
    NULL is returned.
    @param ctx The ID of the context.
    @param index The device index of the capture device.
    @return a pointer to an UTF-8 string containting the name of the capture device.
*/
DLLPUBLIC const char* Cap_getDeviceName(CapContext ctx, CapDeviceID index);

/** Get the unique name of a capture device.
    The string contains a unique concatenation
    of the device name and other parameters.
    These parameters are platform dependent.

    Note: when a USB camera does not expose a serial number,
          platforms might have trouble uniquely identifying 
          a camera. In such cases, the USB port location can
          be used to add a unique feature to the string.
          This, however, has the down side that the ID of
          the camera changes when the USB port location 
          changes. Unfortunately, there isn't much to
          do about this.

    if a device with the given index does not exist,
    NULL is returned.
    @param ctx The ID of the context.
    @param index The device index of the capture device.
    @return a pointer to an UTF-8 string containting the unique ID of the capture device.
*/
DLLPUBLIC const char* Cap_getDeviceUniqueID(CapContext ctx, CapDeviceID index);


/** Returns the number of formats supported by a certain device.
    returns -1 if device does not exist.

    @param ctx The ID of the context.
    @param index The device index of the capture device.
    @return The number of formats supported or -1 if the device does not exist.
*/
DLLPUBLIC int32_t Cap_getNumFormats(CapContext ctx, CapDeviceID index);

/** Get the format information from a device. 
    @param ctx The ID of the context.
    @param index The device index of the capture device.
    @param id The index/ID of the frame buffer format (0 .. number returned by Cap_getNumFormats() minus 1 ).
    @param info pointer to a CapFormatInfo structure to be filled with data.
    @return The CapResult.
*/
DLLPUBLIC CapResult Cap_getFormatInfo(CapContext ctx, CapDeviceID index, CapFormatID id, CapFormatInfo *info); 


/********************************************************************************** 
     STREAM MANAGEMENT
**********************************************************************************/

/** Open a capture stream to a device with specific format requirements 

    Although the (internal) frame buffer format is set via the fourCC ID,
    the frames returned by Cap_captureFrame are always 24-bit RGB.

    @param ctx The ID of the context.
    @param index The device index of the capture device.
    @param formatID The index/ID of the frame buffer format (0 .. number returned by Cap_getNumFormats() minus 1 ).
    @return The stream ID or -1 if the device does not exist or the stream format ID is incorrect.
*/
DLLPUBLIC CapStream Cap_openStream(CapContext ctx, CapDeviceID index, CapFormatID formatID);

/** Close a capture stream 
    @param ctx The ID of the context.
    @param stream The stream ID.
    @return CapResult
*/
DLLPUBLIC CapResult Cap_closeStream(CapContext ctx, CapStream stream);

/** Check if a stream is open, i.e. is capturing data. 
    @param ctx The ID of the context.
    @param stream The stream ID.
    @return 1 if the stream is open and capturing, else 0. 
*/
DLLPUBLIC uint32_t Cap_isOpenStream(CapContext ctx, CapStream stream);

/********************************************************************************** 
     FRAME CAPTURING / INFO
**********************************************************************************/

/** this function copies the most recent RGB frame data
    to the given buffer.
*/
DLLPUBLIC CapResult Cap_captureFrame(CapContext ctx, CapStream stream, void *RGBbufferPtr, uint32_t RGBbufferBytes);

/** returns 1 if a new frame has been captured, 0 otherwise */
DLLPUBLIC uint32_t Cap_hasNewFrame(CapContext ctx, CapStream stream);

/** returns the number of frames captured during the lifetime of the stream. 
    For debugging purposes */
DLLPUBLIC uint32_t Cap_getStreamFrameCount(CapContext ctx, CapStream stream);


/********************************************************************************** 
     NEW CAMERA CONTROL API FUNCTIONS
**********************************************************************************/

/** get the min/max limits and default value of a camera/stream property (e.g. zoom, exposure etc) 

    returns: CAPRESULT_OK if all is well.
             CAPRESULT_PROPERTYNOTSUPPORTED if property not available.
             CAPRESULT_ERR if context, stream are invalid.
*/
DLLPUBLIC CapResult Cap_getPropertyLimits(CapContext ctx, CapStream stream, CapPropertyID propID, 
    int32_t *min, int32_t *max, int *dValue);

/** set the value of a camera/stream property (e.g. zoom, exposure etc) 

    returns: CAPRESULT_OK if all is well.
             CAPRESULT_PROPERTYNOTSUPPORTED if property not available.
             CAPRESULT_ERR if context, stream are invalid.
*/
DLLPUBLIC CapResult Cap_setProperty(CapContext ctx, CapStream stream, CapPropertyID propID, int32_t value);

/** set the automatic flag of a camera/stream property (e.g. zoom, focus etc) 

    returns: CAPRESULT_OK if all is well.
             CAPRESULT_PROPERTYNOTSUPPORTED if property not available.
             CAPRESULT_ERR if context, stream are invalid.
*/
DLLPUBLIC CapResult Cap_setAutoProperty(CapContext ctx, CapStream stream, CapPropertyID propID, uint32_t bOnOff);

/** get the value of a camera/stream property (e.g. zoom, exposure etc) 

    returns: CAPRESULT_OK if all is well.
             CAPRESULT_PROPERTYNOTSUPPORTED if property not available.
             CAPRESULT_ERR if context, stream are invalid or outValue == NULL.
*/
DLLPUBLIC CapResult Cap_getProperty(CapContext ctx, CapStream stream, CapPropertyID propID, int32_t *outValue);

/** get the automatic flag of a camera/stream property (e.g. zoom, focus etc) 

    returns: CAPRESULT_OK if all is well.
             CAPRESULT_PROPERTYNOTSUPPORTED if property not available.
             CAPRESULT_ERR if context, stream are invalid.
*/
DLLPUBLIC CapResult Cap_getAutoProperty(CapContext ctx, CapStream stream, CapPropertyID propID, uint32_t *outValue);

/********************************************************************************** 
     DEBUGGING
**********************************************************************************/

/**
    Set the logging level.

    LOG LEVEL ID  | LEVEL 
    ------------- | -------------
    LOG_EMERG     | 0
    LOG_ALERT     | 1
    LOG_CRIT      | 2
    LOG_ERR       | 3
    LOG_WARNING   | 4
    LOG_NOTICE    | 5
    LOG_INFO      | 6    
    LOG_DEBUG     | 7
    LOG_VERBOSE   | 8

*/
DLLPUBLIC void Cap_setLogLevel(uint32_t level);


typedef void (*CapCustomLogFunc)(uint32_t level, const char *string);

/** install a custom callback for a logging function.

    the callback function must have the following 
    structure:

        void func(uint32_t level, const char *string);
*/
DLLPUBLIC void Cap_installCustomLogFunction(CapCustomLogFunc logFunc);

/** Return the version of the library as a string.
    In addition to a version number, this should 
    contain information on the platform,
    e.g. Win32/Win64/Linux32/Linux64/OSX etc,
    wether or not it is a release or debug
    build and the build date.

    When building the library, please set the 
    following defines in the build environment:

    __LIBVER__
    __PLATFORM__
    __BUILDTYPE__
    
*/

DLLPUBLIC const char* Cap_getLibraryVersion();

#endif

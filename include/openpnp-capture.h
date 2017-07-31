/*

    OpenPnp-Capture: a video capture subsystem.

    Jason von Nieda
    Niels Moseley

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

#define CAPPROPID_EXPOSURE 1
#define CAPPROPID_FOCUS 2
#define CAPPROPID_ZOOM 3
#define CAPPROPID_WHITEBALANCE 4

typedef uint32_t CapPropertyID; ///< property ID (exposure, zoom, focus etc.)

struct CapFormatInfo
{
    uint32_t width;     ///< width in pixels
    uint32_t height;    ///< height in pixels
    uint32_t fourcc;    ///< fourcc code (platform dependent)
    uint32_t fps;       ///< frames per second
    uint32_t bpp;       ///< bits per pixel
};

#define CAPRESULT_OK  0
#define CAPRESULT_ERR 1
#define CAPRESULT_DEVICENOTFOUND 2
#define CAPRESULT_FORMATNOTSUPPORTED 3
#define CAPRESULT_PROPERTYNOTSUPPORTED 4

/********************************************************************************** 
     CONTEXT CREATION AND DEVICE ENUMERATION
**********************************************************************************/

/** initialize the capture library */
DLLPUBLIC CapContext Cap_createContext(void);

/** un-initialize the capture library */
DLLPUBLIC CapResult Cap_releaseContext(CapContext ctx);

/** get the number of capture devices on the system.
    note: this can change dynamically due to the
    pluggin and unplugging of USB devices */
DLLPUBLIC uint32_t Cap_getDeviceCount(CapContext ctx);

/** get the name of a capture device.
    Note: this name may or may not be unique
    or persistent across system reboots. 

    if a device with the given index does not exist,
    NULL is returned.
*/
DLLPUBLIC const char* Cap_getDeviceName(CapContext ctx, CapDeviceID index);

/** return the number of formats supported by a certain device.
    returns -1 if device does not exist.
*/
DLLPUBLIC int32_t Cap_getNumFormats(CapContext ctx, CapDeviceID index);

/** get the format information from a device. */
DLLPUBLIC CapResult Cap_getFormatInfo(CapContext ctx, CapDeviceID index, CapFormatID id, CapFormatInfo *info); 


/********************************************************************************** 
     STREAM MANAGEMENT
**********************************************************************************/

/** open a capture stream to a device with specific format requirements 

    If the device is not capable of the requirements or the device
    does not exits, -1 is returned.

    Although the (internal) frame buffer format is set via the fourCC ID,
    the frames returned by Cap_captureFrame are always 24-bit RGB.
*/
DLLPUBLIC CapStream Cap_openStream(CapContext ctx, CapDeviceID index, CapFormatID formatID);

/** close a capture stream */
DLLPUBLIC CapResult Cap_closeStream(CapContext ctx, CapStream stream);

/** returns 1 if the stream is open and capturing, else 0 */
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

/** get the min/max limits of a camera/stream property (e.g. zoom, exposure etc) */
DLLPUBLIC CapResult Cap_getPropertyLimits(CapContext ctx, CapStream stream, CapPropertyID propID, int32_t *min, int32_t *max);

/** set the value of a camera/stream property (e.g. zoom, exposure etc) */
DLLPUBLIC CapResult Cap_setProperty(CapContext ctx, CapStream stream, CapPropertyID propID, int32_t value);

/** set the automatic flag of a camera/stream property (e.g. zoom, focus etc) */
DLLPUBLIC CapResult Cap_setAutoProperty(CapContext ctx, CapStream stream, CapPropertyID propID, uint32_t bOnOff);

/********************************************************************************** 
     DEBUGGING
**********************************************************************************/

DLLPUBLIC void Cap_setLogLevel(uint32_t level);

/** Return the version of the library as a string.
    In addition to a version number, this should 
    contain information on the platform,
    e.g. Win32/Win64/Linux32/Linux64/OSX etc,
    wether or not it is a release or debug
    build and the build date.

    When building the library, please set the 
    following defines in the build environment:

    __VERSION__
    __PLATFORM__
    __BUILDTYPE__
    
*/

DLLPUBLIC const char* Cap_getLibraryVersion();

#endif

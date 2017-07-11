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

#define CAPRESULT_OK  0
#define CAPRESULT_ERR 1
#define CAPRESULT_DEVICENOTFOUND 2
#define CAPRESULT_FORMATNOTSUPPORTED 3
#define CAPRESULT_EXPOSURENOTSUPPORTED 4
#define CAPRESULT_FOCUSNOTSUPPORTED 5

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

/** open a capture stream to a device with specific format requirements 

    Note: if the device is not capable of the requirements or the device
    does not exits, -1 is returned.
*/
DLLPUBLIC CapStream Cap_openStream(CapContext ctx, CapDeviceID index, uint32_t width, uint32_t height, uint32_t fourCC);

/** close a capture stream */
DLLPUBLIC CapResult Cap_closeStream(CapContext ctx, CapStream stream);

/** returns 1 if the stream is open and capturing, else 0 */
DLLPUBLIC uint32_t Cap_isOpenStream(CapContext ctx, CapStream stream);

/** this function copies the most recent RGB frame data
    to the given buffer.
*/
DLLPUBLIC CapResult Cap_captureFrame(CapContext ctx, CapStream stream, void *RGBbufferPtr, uint32_t RGBbufferBytes);

/** returns 1 if a new frame has been captured, 0 otherwise */
DLLPUBLIC uint32_t Cap_hasNewFrame(CapContext ctx, CapStream stream);

/** returns the number of frames captured during the lifetime of the stream. 
    For debugging purposes */
DLLPUBLIC uint32_t Cap_getStreamFrameCount(CapContext ctx, CapStream stream);

DLLPUBLIC CapResult Cap_getExposureLimits(CapContext ctx, CapStream stream, int32_t *min, int32_t *max);
DLLPUBLIC CapResult Cap_setExposure(CapContext ctx, CapStream stream, int32_t value);
DLLPUBLIC CapResult Cap_setAutoExposure(CapContext ctx, CapStream stream, uint32_t bOnOff);

DLLPUBLIC void Cap_setLogLevel(uint32_t level);

#endif

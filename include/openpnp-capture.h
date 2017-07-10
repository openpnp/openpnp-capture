/*

    OpenPnp-Capture: a video capture subsystem.

    Jason von Nieda
    Niels Moseley

*/

#ifndef openpnp_capture_h
#define openpnp_capture_h

#include <stdint.h>

// make sure its exported/imported as pure C 
// even if we're compiling with a C++ compiler
#ifdef BUILD_OPENPNP_LIBRARY
    #ifdef __cplusplus
        #define DLLEXPORT extern "C" __declspec(dllexport)
    #else
        #define DLLEXPORT __declspec(dllexport)
    #endif
#else
    #ifdef __cplusplus
        #define DLLEXPORT extern "C" __declspec(dllimport)
    #else
        #define DLLEXPORT __declspec(dllimport)
    #endif
#endif

typedef void* CapContext;
typedef int32_t  CapStream;
typedef uint32_t CapResult;
typedef uint32_t CapDeviceID;

#define CAPRESULT_OK  0
#define CAPRESULT_ERR 1
#define CAPRESULT_DEVICENOTFOUND 2
#define CAPRESULT_FORMATNOTSUPPORTED 3

/** initialize the capture library */
DLLEXPORT CapContext Cap_createContext(void);

/** un-initialize the capture library */
DLLEXPORT CapResult Cap_releaseContext(CapContext ctx);

/** get the number of capture devices on the system.
    note: this can change dynamically due to the
    pluggin and unplugging of USB devices */
DLLEXPORT uint32_t Cap_getDeviceCount(CapContext ctx);

/** get the name of a capture device.
    Note: this name may or may not be unique
    or persistent across system reboots. 

    if a device with the given index does not exist,
    NULL is returned.
*/
DLLEXPORT const char* Cap_getDeviceName(CapContext ctx, CapDeviceID index);

/** open a capture stream to a device with specific format requirements 

    Note: if the device is not capable of the requirements or the device
    does not exits, -1 is returned.
*/
DLLEXPORT CapStream Cap_openStream(CapContext ctx, CapDeviceID index, uint32_t width, uint32_t height, uint32_t fourCC);

/** close a capture stream */
DLLEXPORT CapResult Cap_closeStream(CapContext ctx, CapStream stream);

/** returns 1 if the stream is open and capturing, else 0 */
DLLEXPORT uint32_t Cap_isOpenStream(CapContext ctx, CapStream stream);

/** this function copies the most recent RGB frame data
    to the given buffer.
*/
DLLEXPORT CapResult Cap_captureFrame(CapContext ctx, CapStream stream, void *RGBbufferPtr, uint32_t RGBbufferBytes);

/** returns 1 if a new frame has been captured, 0 otherwise */
DLLEXPORT uint32_t Cap_hasNewFrame(CapContext ctx, CapStream stream);

/** returns the number of frames captured during the lifetime of the stream. 
    For debugging purposes */
DLLEXPORT uint32_t Cap_getStreamFrameCount(CapContext ctx, CapStream stream);

DLLEXPORT CapResult Cap_getExposureLimits(CapDeviceID index, float *min, float *max);
DLLEXPORT CapResult Cap_setExposure(CapDeviceID index, float value);
DLLEXPORT CapResult Cap_setAutoExposure(CapStream stream, uint32_t bOnOff);

DLLEXPORT void Cap_setLogLevel(uint32_t level);

#endif

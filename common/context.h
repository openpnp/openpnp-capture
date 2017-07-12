/*

    OpenPnp-Capture: a video capture subsystem.

    Created by Niels Moseley on 7/11/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Platform independent context class to keep track
    of the global state.

*/

#ifndef openpnp_context_h
#define openpnp_context_h

#define _CRT_SECURE_NO_WARNINGS

#include <vector>
#include <string>
#include <map>
#include <stdint.h>

#include "openpnp-capture.h"
#include "deviceinfo.h"

class Stream;   // pre-declaration

/* Define a platform stream factory call to
   separate platform dependent code from this class.
   This function needs to be provided in platformstream.cpp 
*/
Stream* createPlatformStream();

/** context base class keeps track of all the platform independent
    objects and information */

class Context
{
public:
    /** Create a context for the library.
        Device enumeration is perform in the constructor,
        so all devices must be present in the system when
        the Context is created or devices will not be found.

        Re-enumeration support is pending.
    */
    Context();
    virtual ~Context();

    /** Get the UTF-8 device name of a device with index/ID id */
    const char* getDeviceName(CapDeviceID id) const;

    /** Return the number of devices found */
    uint32_t getDeviceCount() const;

    /** return the number of formats supported by a certain device */
    int32_t getNumFormats(CapDeviceID index) const;

    /** get the format information from a device. */
    bool getFormatInfo(CapDeviceID index, CapFormatID id, CapFormatInfo *info) const;

    /** Opens a stream to a device with index/ID id and returns the stream ID.
        If an error occurs (device not found), -1 is returned.

        If the stream is succesfully opnened, capturing starts automatically
        until the stream (or its associated context) is closed with closeStream.

        Note: for now, only one stream per device is supported but opening more
              streams might or might not work.
    */
    int32_t openStream(CapDeviceID id, CapFormatID formatID);

    /** close the stream to a device */
    bool closeStream(int32_t streamID);

    /** returns 1 if the stream is open and capturing, else 0 */
    uint32_t isOpenStream(int32_t streamID);

    /** returns true if succeeds, else false */
    bool captureFrame(int32_t streamID, uint8_t *RGBbufferPtr, size_t RGBbufferBytes);

    /** returns true if the stream has a new frame, false otherwise */
    bool hasNewFrame(int32_t streamID);

    /** returns the number of frames captured during the lifetime of the stream */
    uint32_t getStreamFrameCount(int32_t streamID);

    /** set the exposure of the camera in 'camera' units. */
    bool setStreamExposure(int32_t streamID, int32_t value);

    /** enable/disable the automatic exposure setting in the camera. */
    bool setStreamAutoExposure(int32_t streamID, bool enable);

    /** get the min and max exposure settings in 'camera' units. */
    bool getStreamExposureLimits(int32_t streamID, int32_t *min, int32_t *max);

protected:
    /** Enumerate all capture devices and put their 
        information (name, buffer formats etc) into 
        the m_devices array.
        
        Implement this function in a platform-dependent
        derived class.
    */
    virtual bool enumerateDevices() = 0;

    /** Lookup a stream by ID and return a pointer
        to it if it exists. If it doesnt exist, 
        return NULL */
    Stream* lookupStreamByID(int32_t ID);

    /** Store a stream pointer in the m_streams map
        and return its unique ID */
    int32_t storeStream(Stream *stream);

    /** Remove a stream from the m_streams map.
        Return true if this was successful */
    bool removeStream(int32_t ID);

    std::vector<deviceInfo*>    m_devices;          ///< list of enumerated devices
    std::map<int32_t, Stream*>  m_streams;          ///< collection of streams
    int32_t                     m_streamCounter;    ///< counter to generate stream IDs
};

/** convert a FOURCC uint32_t to human readable form */
std::string fourCCToString(uint32_t fourcc);

#endif
/*

    OpenPnp-Capture: a video capture subsystem.

    Platform independent context class to keep track of the global state.

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

#ifndef openpnp_context_h
#define openpnp_context_h

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


    /** Get the UTF-8 unique name of a capture device.
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
        @param id The device index of the capture device.
        @return a pointer to an UTF-8 string containting the unique ID of the capture device.
    */
    const char* getDeviceUniqueID(CapDeviceID id) const;

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

    /** set the frame rate of a stream 
        returns false if the camera does not support the frame rate
    */
    bool setStreamFrameRate(int32_t streamID, uint32_t fps);

    /** Get the minimum and maximum settings for a property.
        @param streamID the ID of the stream.
        @param propertyID the ID of the property.
        @param min a pointer to an int32_t that will receive the minimum setting.
        @param max a pointer to an int32_t that will receive the maximum setting.
        @param dValue a pointer to an int32_t that will receive the default value for the setting.
        @return true if min, max and dValue were written.
    */
    bool getStreamPropertyLimits(int32_t streamID, uint32_t propertyID, 
            int32_t *min, int32_t *max, int32_t *dValue);

    /** Turn on or off a property that support an automatic setting,
        such as exposure or white balance.

        @param streamID the ID of the stream.
        @param propertyID the ID of the property.
        @param enable the desired new state of the auto setting.
        @return true if succesful.
    */    
    bool setStreamAutoProperty(int32_t streamID, uint32_t propertyID, bool enable);

    /** Set the value of a property, such as exposure or white balance.

        @param streamID the ID of the stream.
        @param propertyID the ID of the property.
        @param value the new value of the property.
        @return true if succesful.
    */        
    bool setStreamProperty(int32_t streamID, uint32_t propertyID, int32_t value);


    /** Get the value of a property, such as exposure or white balance.

        @param streamID the ID of the stream.
        @param propretyID the ID of the property.
        @param outValue a reference to the int32_t that will receive the value of the property.
        @return true if succesful.
    */
    bool getStreamProperty(int32_t stream, uint32_t propID, int32_t &outValue);

    /** Get the value of a property, such as exposure or white balance.

        @param streamID the ID of the stream.
        @param propretyID the ID of the property.
        @param enable a reference to a boolean that will receive the state of the auto setting.
        @return true if succesful.
    */
    bool getStreamAutoProperty(int32_t stream, uint32_t propID, bool &enable);

protected:
    /** Enumerate all capture devices and put their 
        information (name, buffer formats etc) into 
        the m_devices array.
        
        Implement this function in a platform-dependent
        derived class.
    */
    virtual bool enumerateDevices() = 0;

    /** Store a stream pointer in the m_streams map
        and return its unique ID */
    int32_t storeStream(Stream *stream);

    /** Remove a stream from the m_streams map
        and call delete on the object.
        Return true if this was successful */
    bool removeStream(int32_t ID);

    std::vector<deviceInfo*>    m_devices;          ///< list of enumerated devices
    std::map<int32_t, Stream*>  m_streams;          ///< collection of streams
    int32_t                     m_streamCounter;    ///< counter to generate stream IDs
};

/** convert a FOURCC uint32_t to human readable form */
std::string fourCCToString(uint32_t fourcc);

#endif
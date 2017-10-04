/*

    OpenPnp-Capture: a video capture subsystem.

    Platform independent stream class

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

#ifndef stream_h
#define stream_h

#include <stdint.h>
#include <vector>
#include <mutex>
#include "logging.h"

class Context;      // pre-declaration
class deviceInfo;   // pre-declaration
class Stream;       // pre-declaration


/** The stream class handles the capturing of a single device */
class Stream
{
public:
    Stream();
    virtual ~Stream();

    /** Open a capture stream to a device and request a specific (internal) stream format. 
        When succesfully opened, capturing starts immediately.
    */
    virtual bool open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, 
        uint32_t fourCC, uint32_t fps) = 0;

    /** Close a capture stream */
    virtual void close() {};

    /** Returns true if a new frame is available for reading using 'captureFrame'. 
        The internal new frame flag is reset by captureFrame.
    */
    bool hasNewFrame();

    /** Retrieve the most recently captured frame and copy it in a
        buffer pointed to by RGBbufferPtr. The maximum buffer size 
        must be supplied in RGBbufferBytes.
    */
    bool captureFrame(uint8_t *RGBbufferPtr, uint32_t RGBbufferBytes);
    
    /** Set the frame rate of this stream.
        Returns false if the camera does not support the desired
        frame rate.
    */
    virtual bool setFrameRate(uint32_t fps) = 0;

    /** Returns true if the stream is open and capturing */
    bool isOpen() const
    {
        return m_isOpen;
    }

    /** Return the FOURCC media type of the stream */
    virtual uint32_t getFOURCC() = 0;

    /** Return the number of frames captured.
        FIXME: protect by mutex 
    */
    uint32_t getFrameCount() const
    {
        return m_frames;
    }

    /** get the limits of a camera/stream property (exposure, zoom etc) */
    virtual bool getPropertyLimits(uint32_t propID, int32_t *min, int32_t *max, int32_t *dValue) = 0;

    /** set property (exposure, zoom etc) of camera/stream */
    virtual bool setProperty(uint32_t propID, int32_t value) = 0;

    /** set automatic state of property (exposure, zoom etc) of camera/stream */
    virtual bool setAutoProperty(uint32_t propID, bool enabled) = 0;

    /** get property (exposure, zoom etc) of camera/stream */
    virtual bool getProperty(uint32_t propID, int32_t &outValue) = 0;

    /** get automatic state of property (exposure, zoom etc) of camera/stream */
    virtual bool getAutoProperty(uint32_t propID, bool &enable) = 0;

protected:
    /** Thread-safe copying of the 24-bit RGB buffer pointed to
        by 'ptr' with length 'bytes'.

        If no bitmap conversion needs to take place, call this
        function from the platform dependent code. Otherwise, re-
        implement this function with the conversion to avoid
        needing multiple frame buffers.

    */
    virtual void submitBuffer(const uint8_t* ptr, size_t bytes);

    Context*    m_owner;                    ///< The context object associated with this stream

    uint32_t    m_width;                    ///< The width of the frame in pixels
    uint32_t    m_height;                   ///< The height of the frame in pixels
    bool        m_isOpen;

    std::mutex  m_bufferMutex;              ///< mutex to protect m_frameBuffer and m_newFrame
    bool        m_newFrame;                 ///< new frame buffer flag
    std::vector<uint8_t> m_frameBuffer;     ///< raw frame buffer
    uint32_t    m_frames;                   ///< number of frames captured
};

#endif
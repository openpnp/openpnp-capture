/*

    OpenPnp-Capture: a video capture subsystem.

    Platform independent stream class

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

*/

#ifndef stream_h
#define stream_h

#include <stdint.h>
#include <vector>
#include <mutex>
#include "logging.h"

class Context;      // pre-declaration
struct deviceInfo;  // pre-declaration
class Stream;       // pre-declaration


/** The stream class handles the capturing of a single device */
class Stream
{
public:
    Stream();
    ~Stream();

    /** Open a capture stream to a device and request a specific (internal) stream format. 
        When succesfully opened, capturing starts immediately.
    */
    virtual bool open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, uint32_t fourCC) = 0;

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

protected:
    void submitBuffer(uint8_t* ptr, size_t bytes);

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
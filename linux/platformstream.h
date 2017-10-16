/*

    OpenPnp-Capture: a video capture subsystem.

    Linux platform code
    Stream class

    Created by Niels Moseley on 7/6/17.
    Copyright (c) 2017 Niels Moseley.

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

#ifndef linux_platformstream_h
#define linux_platformstream_h

#include <stdint.h>
#include <vector>
#include <mutex>
#include <thread>
#include <linux/videodev2.h>
#include "../common/logging.h"
#include "../common/stream.h"
#include "mjpeghelper.h"


class Context;          // pre-declaration
class PlatformStream;  // pre-declaration


/** A helper class to take care of allocation and
    de-allocation of memory mapped V4L2 buffers */
class PlatformStreamHelper
{
public:
    PlatformStreamHelper(int fd) : m_fd(fd)
    {
        LOG(LOG_DEBUG, "PlatformStreamHelper created.\n");
    }

    virtual ~PlatformStreamHelper()
    {
        if (m_buffers.size() != 0)
        {
            streamOff();
            unmapAndDeleteBuffers();
        }
        LOG(LOG_DEBUG, "PlatformStreamHelper deleted.\n");
    }

    /** remove the memory mapped buffers from the system */
    void unmapAndDeleteBuffers();

    /** create a number of memory mapped buffers */
    bool createAndMapBuffers(uint32_t nBuffers);

    /** queue all the buffer for use by V4L2 */
    bool queueAllBuffers();

    /** tell V4L2 to start frame capturing */
    bool streamOn();

    /** tell V4L2 to stop frame capturing */
    bool streamOff();

    /** return a pointer to the buffer with a certain index
        in m_buffers vector */
    void* getBufferPointer(uint32_t index) const
    {
        if (index < m_buffers.size())
        {
            return m_buffers[index].start;
        }
        else
        {
            return nullptr;
        }
    }

    struct bufferInfo
    {
        void*   start;      // pointer to start of buffer
        size_t  length;     // length of buffer in bytes
    };

    std::vector<bufferInfo> m_buffers;
    int m_fd; 
};


/** Platform dependent code to support Linux using V4L2 */
class PlatformStream : public Stream
{
public:
    PlatformStream();
    virtual ~PlatformStream();

    /** Open a capture stream to a device and request a specific (internal) stream format. 
        When succesfully opened, capturing starts immediately.
    */
    virtual bool open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, 
        uint32_t fourCC, uint32_t fps) override;

    /** Close a capture stream */
    virtual void close() override;

    /** Return the FOURCC media type of the stream */
    virtual uint32_t getFOURCC() override;

    virtual bool setProperty(uint32_t propID, int32_t value) override;
    virtual bool setAutoProperty(uint32_t propID, bool enabled) override;
    virtual bool getPropertyLimits(uint32_t propID, 
        int32_t *min, int32_t *max, int32_t *dValue) override;

    virtual bool getProperty(uint32_t propID, int32_t &value) override;
    virtual bool getAutoProperty(uint32_t propID, bool &enabled) override;

    virtual bool setFrameRate(uint32_t fps) override;

    /** called by the capture thread/function to query if it
        should quit */
    bool getThreadQuitState() const
    {
        return m_quitThread;
    }

    /** public submit buffer so the capture thread/function
        can access it. In additon, this function handles any 
        conversion to RGB output buffers, if necessary */
    void threadSubmitBuffer(void *ptr, size_t bytes);

protected:
    int         m_deviceHandle;     ///< V4L2 device handle
    v4l2_format m_fmt;              ///< V4L2 frame format
    bool        m_quitThread;       ///< if true, captureThreadFunction should return
    std::thread *m_helperThread;    ///< helper object threading control
    MJPEGHelper m_mjpegHelper;      ///< helper to convert MJPEG stream to RGB
};

#endif
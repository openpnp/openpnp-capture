/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Stream class

*/

#ifndef win32platform_stream_h
#define win32platform_stream_h

#include <stdint.h>
#include <vector>
#include <mutex>
#include <thread>
#include <linux/videodev2.h>
#include "../common/logging.h"
#include "../common/stream.h"


class Context;          // pre-declaration
class PlatformStream;  // pre-declaration


class PlatformStream : public Stream
{
public:
    PlatformStream();
    virtual ~PlatformStream();

    /** Open a capture stream to a device and request a specific (internal) stream format. 
        When succesfully opened, capturing starts immediately.
    */
    virtual bool open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, uint32_t fourCC) override;

    /** Close a capture stream */
    virtual void close() override;

    /** Return the FOURCC media type of the stream */
    virtual uint32_t getFOURCC() override;

    virtual bool setExposure(int32_t value) override;

    virtual bool setAutoExposure(bool enabled) override;

    virtual bool getExposureLimits(int32_t *min, int32_t *max) override;

    /** called by the capture thread/function to query if it
        should quit */
    bool getThreadQuitState() const
    {
        return m_quitThread;
    }

    /** public submit buffer so the capture thread/function
        can access it */
    void threadSubmitBuffer(uint8_t *ptr, size_t bytes)
    {
        submitBuffer(ptr, bytes);
    }

protected:
    /** generate FOURCC string from a uint32 */
    std::string genFOURCCstring(uint32_t v);

    int         m_deviceHandle;     ///< V4L2 device handle
    v4l2_format m_fmt;              ///< V4L2 frame format
    bool        m_quitThread;       ///< if true, captureThreadFunction should return
    std::thread *m_helperThread;    ///< helper object threading control
};

#endif
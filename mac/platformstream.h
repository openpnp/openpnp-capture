/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Stream class

*/

#ifndef macplatform_stream_h
#define macplatform_stream_h

#include <stdint.h>
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

    /** get the limits of a camera/stream property (exposure, zoom etc) */
    virtual bool getPropertyLimits(uint32_t propID, int32_t *min, int32_t *max) override;
    
    /** set property (exposure, zoom etc) of camera/stream */
    virtual bool setProperty(uint32_t propID, int32_t value) override;

    /** set automatic state of property (exposure, zoom etc) of camera/stream */
    virtual bool setAutoProperty(uint32_t propID, bool enabled) override;

protected:
    /** generate FOURCC string from a uint32 */
    std::string genFOURCCstring(uint32_t v);
};

#endif

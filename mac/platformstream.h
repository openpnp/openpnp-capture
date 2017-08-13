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
#import <AVFoundation/AVFoundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

class Context;          // pre-declaration
class PlatformStream;  // pre-declaration

/** An OBJC++ interface to handle callbacks from the
    video sub-system on OSX */
@interface PlatformAVCaptureDelegate :
NSObject<AVCaptureVideoDataOutputSampleBufferDelegate>
{
@public
    PlatformStream *m_stream;
}
- (void)captureOutput:(AVCaptureOutput *)out
    didDropSampleBuffer:(CMSampleBufferRef)sampleBuffer
    fromConnection:(AVCaptureConnection *)connection;
- (void)captureOutput:(AVCaptureOutput *)captureOutput
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
    fromConnection:(AVCaptureConnection *)connection;
@end


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

    /** public function to handle callbacks from ObjC++ */
    virtual void callback(const uint8_t* ptr, uint32_t bytes);

protected:
    PlatformAVCaptureDelegate* m_captureDelegate;
    AVCaptureSession*   m_nativeSession;
    dispatch_queue_t    m_queue;

    std::vector<uint8_t> m_tmpBuffer;  ///< intermediate buffer for 32->24 bit conversion

    /** generate FOURCC string from a uint32 */
    std::string genFOURCCstring(uint32_t v);
};

#endif

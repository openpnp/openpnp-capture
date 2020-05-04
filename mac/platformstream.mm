/*

    OpenPnp-Capture: a video capture subsystem.

    OSX platform code
    Stream class

    Created by Niels Moseley on 7/6/17.
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

#include "platformdeviceinfo.h"
#include "platformstream.h"
#include "platformcontext.h"
#include <Accelerate/Accelerate.h>

// **********************************************************************
//   ObjC++ callback handler implementation
// **********************************************************************

@implementation PlatformAVCaptureDelegate
- (void)captureOutput:(AVCaptureOutput *)out
        didDropSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection
{

}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection
{

    // check the number of samples/frames
	CMItemCount count = CMSampleBufferGetNumSamples(sampleBuffer);
	if (count < 1)
    {
		return;
    }

    // sanity check on the stream pointer
    if (m_stream != nullptr)
    {
        CMFormatDescriptionRef desc = CMSampleBufferGetFormatDescription(sampleBuffer);
        FourCharCode fourcc = CMFormatDescriptionGetMediaSubType(desc);
        CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(desc);

        #if 0
        // generate 4cc string
        char fourCCString[5];
        for(uint32_t i=0; i<4; i++)
        {
            fourCCString[i] = static_cast<char>(fourcc & 0xFF);
            fourcc >>= 8;
        }
        fourCCString[4] = 0;
        LOG(LOG_DEBUG, "%d x %d %s\n", dims.width, dims.height, fourCCString);
        #endif


        // https://stackoverflow.com/questions/34569750/get-pixel-value-from-cvpixelbufferref-in-swift
        // get lock to pixel buffer so we can read the actual frame buffer data
        CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
        if (CVPixelBufferLockBaseAddress(pixelBuffer, 0) == kCVReturnSuccess)
        {
            const uint8_t *pixelPtr = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixelBuffer));
            uint32_t frameBytes = CVPixelBufferGetHeight(pixelBuffer) *
                  CVPixelBufferGetBytesPerRow(pixelBuffer);

            m_stream->callback(pixelPtr, frameBytes);

            CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
        }
    }
}

@end


// **********************************************************************
//   PlatformStream implementation
// **********************************************************************

Stream* createPlatformStream()
{
    return new PlatformStream();
}

PlatformStream::PlatformStream() :
    Stream()
{
    m_uvc = nullptr;
    m_fourCC = 0;
    m_nativeSession = nullptr;
}

PlatformStream::~PlatformStream()
{
    if (m_uvc != nullptr)
    {
        delete m_uvc;
        m_uvc = nullptr;
    }
    close();
}

void PlatformStream::close()
{
    LOG(LOG_INFO, "closing stream\n");
    
    if (m_nativeSession != nullptr)
    {
        [m_nativeSession stopRunning];
        [m_nativeSession release];
        m_nativeSession = nullptr;
    }

    m_fourCC = 0;
    m_isOpen = false;
    m_device = nullptr; // note: we don't own the device object!
}

bool PlatformStream::open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, 
    uint32_t fourCC, uint32_t fps)
{
    if (m_isOpen)
    {
        LOG(LOG_INFO,"open() was called on an active stream.\n");
        close();
    }

    if (owner == nullptr)
    {
        LOG(LOG_ERR,"open() was with owner=NULL!\n");        
        return false;
    }

    if (device == nullptr)
    {
        LOG(LOG_ERR,"open() was with device=NULL!\n");
        return false;
    }

    platformDeviceInfo *dinfo = dynamic_cast<platformDeviceInfo*>(device);
    if (dinfo == NULL)
    {
        LOG(LOG_CRIT, "Could not cast deviceInfo* to platfromDeviceInfo*!\n");
        return false;
    }

    if (dinfo->m_captureDevice == nullptr)
    {
        LOG(LOG_CRIT, "m_captureDevice is a NULL pointer!\n");
        return false;        
    }

    //copy the device pointer into stream object
    m_device = (__bridge AVCaptureDevice*) dinfo->m_captureDevice;

    // create a new session manager and open a capture session
    m_nativeSession = [AVCaptureSession new];

    NSError* error = nil;
    AVCaptureDeviceInput* input = [AVCaptureDeviceInput deviceInputWithDevice:m_device error:&error];
    if (!input) 
    {
        LOG(LOG_ERR, "Error opening native device %s\n", error.localizedDescription.UTF8String);
        return false;
    }

    // add the device to the session object
    // it seems this must go before everything else!
    [m_nativeSession addInput:input];

    LOG(LOG_DEBUG, "Setup for capture format (%d x %d)...\n", width, height);

    AVCaptureDeviceFormat *bestFormat = nil;
    for(uint32_t i=0; i<dinfo->m_platformFormats.size(); i++)
    {
        CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(dinfo->m_platformFormats[i].formatDescription);
        if ((dims.width == width) && (dims.height == height))
        {
            // check fourCC
            uint32_t myFourCC = CMFormatDescriptionGetMediaSubType(dinfo->m_platformFormats[i].formatDescription);
            if (myFourCC == fourCC)
            {
                bestFormat = dinfo->m_platformFormats[i];
                m_fourCC = myFourCC;
                break;
            }
        } 
    }

    if (bestFormat == nil)
    {
        LOG(LOG_ERR,"could not find a suitable format\n");
        return false;
    }
    
    //FIXME: error checking..
    [m_device lockForConfiguration:NULL];
    m_device.activeFormat = bestFormat;
    
    m_width = width;
    m_height = height;
    m_owner = owner;
    m_frameBuffer.resize(m_width*m_height*3);
    m_tmpBuffer.resize(m_width*m_height*3);

    AVCaptureVideoDataOutput* output = [AVCaptureVideoDataOutput new];
    [m_nativeSession addOutput:output];
    output.videoSettings = nil;

    output.videoSettings = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32ARGB], (id)kCVPixelBufferPixelFormatTypeKey,
        nil];

    // discard data if the output queue is blocked
    [output setAlwaysDiscardsLateVideoFrames:true];

    // a serial dispatch queue must be used to guarantee that video frames will be delivered in order
    m_queue = dispatch_queue_create("VideoDataOutputQueue", DISPATCH_QUEUE_SERIAL);

    // create the callback handling delegate
    m_captureDelegate = [PlatformAVCaptureDelegate new];
    if (m_captureDelegate == nullptr)
    {
        LOG(LOG_ERR, "cannot create PlatformAVCaptureDelegate\n.");
        return false;
    }

    // register the stream with the callback delegate so it can be
    // called throug the stream->callback() method 
    m_captureDelegate->m_stream = this;
    [output setSampleBufferDelegate:m_captureDelegate queue:m_queue];

    // start capturing
    [m_nativeSession startRunning];

    // unlock must be after startRunning call
    // otherwise the session object will
    // override our settings :-/
    [m_device unlockForConfiguration];

    // try to create a UVC control object
    m_uvc = UVCCtrl::create(dinfo->m_vid, dinfo->m_pid, dinfo->m_busLocation);
    if (m_uvc != nullptr)
    {
        LOG(LOG_DEBUG, "Created a UVC control object!\n");
    }
    else
    {
        LOG(LOG_DEBUG, "Could not create a UVC control object! -- settings will not be available!\n");
    }

    m_isOpen = true;
    m_frames = 0; // reset the frame counter
    return true;
}

std::string PlatformStream::genFOURCCstring(uint32_t v)
{
    std::string result;
    for(uint32_t i=0; i<4; i++)
    {
        result += static_cast<char>(v & 0xFF);
        v >>= 8;
    }
    return result;
}

/** get the limits of a camera/stream property (exposure, zoom etc) */
bool PlatformStream::getPropertyLimits(uint32_t propID, int32_t *min, int32_t *max, int32_t *dValue)
{
    if ((m_uvc != nullptr) && (min != nullptr) && (max != nullptr) && (dValue != nullptr))
    {
        LOG(LOG_INFO,"PlatformStream::getPropertyLimits\n");
        return m_uvc->getPropertyLimits(propID, min, max, dValue);
    }
    return false;
}

bool PlatformStream::setFrameRate(uint32_t fps)
{
    //FIXME: unsupported
    return false;
}

uint32_t PlatformStream::getFOURCC()
{
    //note: OSX stores the fourcc in reverse byte order
    // compared to Windows or Linux so we swap the 
    // bytes here to be compatible

    uint32_t tmp = m_fourCC;
    tmp = ((tmp >> 16) & 0x0000ffff) | ((tmp << 16) & 0xffff0000);
    tmp = ((tmp & 0x00ff00ff)<<8) | ((tmp & 0xff00ff00)>>8);

    return tmp;
}

/** set property (exposure, zoom etc) of camera/stream */
bool PlatformStream::setProperty(uint32_t propID, int32_t value)
{
    if (m_uvc == nullptr)
    {
        return false;
    }

    return m_uvc->setProperty(propID, value);
}

/** set automatic state of property (exposure, zoom etc) of camera/stream */
bool PlatformStream::setAutoProperty(uint32_t propID, bool enabled)
{
    if (m_uvc == nullptr) 
    {
        return false;
    }

    return m_uvc->setAutoProperty(propID, enabled);
}

// FIXME: properties are not properly supported on OSX and must be
//        implemented using direct access of UVC cameras
bool PlatformStream::getProperty(uint32_t propID, int32_t &value)
{
    if (m_uvc == nullptr)
    {
        return false;
    }

    int32_t v;
    bool ok = m_uvc->getProperty(propID, &v);
    value = v;
    return ok;
}

// FIXME: properties are not properly supported on OSX and must be
//        implemented using direct access of UVC cameras
bool PlatformStream::getAutoProperty(uint32_t propID, bool &enabled)
{
    if (m_uvc == nullptr) 
    {
        return false;
    }

    bool e;
    bool ok = m_uvc->getAutoProperty(propID, &e);
    enabled = e;
    return ok;
}

void PlatformStream::callback(const uint8_t *ptr, uint32_t bytes)
{
    // here we get 32-bit ARGB buffers, which we need to
    // convert to 24-bit RGB buffers
    
    if (m_tmpBuffer.size() != (m_width*m_height*3))
    {
        // error: temporary buffer is not the right size!
        return;
    }

    vImage_Buffer src;
    vImage_Buffer dst;
    src.data = (void*)ptr;  // fugly
    src.width = m_width;
    src.height = m_height;
    src.rowBytes = m_width*4;
    dst.data = &m_tmpBuffer[0];
    dst.width = m_width;
    dst.height = m_height;
    dst.rowBytes = m_width*3;

    vImageConvert_ARGB8888toRGB888(&src, &dst, 0);
    submitBuffer(&m_tmpBuffer[0], m_tmpBuffer.size());
}

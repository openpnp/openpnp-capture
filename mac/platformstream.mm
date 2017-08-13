#include "platformdeviceinfo.h"
#include "platformstream.h"
#include "platformcontext.h"

Stream* createPlatformStream()
{
    return new PlatformStream();
}

PlatformStream::PlatformStream() :
    Stream()
{

}

PlatformStream::~PlatformStream()
{
    close();
}

void PlatformStream::close()
{
    LOG(LOG_INFO, "closing stream\n");
}

bool PlatformStream::open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, uint32_t fourCC)
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
        LOG(LOG_CRIT, "Could not cast deviceInfo* to platfromDeviceInfo*!");
        return false;
    }

    return true;
}

bool PlatformStream::getPropertyLimits(uint32_t propID, int32_t *min, int32_t *max) 
{
    return false;
}

bool PlatformStream::setProperty(uint32_t propID, int32_t value) 
{
    return false;
}

bool PlatformStream::setAutoProperty(uint32_t propID, bool enabled)
{
    return false;
}

uint32_t PlatformStream::getFOURCC()
{
    return 0;
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

//capture_status open_session(capture_device* device, capture_session** session) {
//    AVCaptureDevice* native_device = (__bridge AVCaptureDevice*) (void*) device->_internal;
//    
//    NSLog(@"open_session(%@, %p)", native_device, session);
//    
//    Session* native_session = [Session new];
//    
//    native_session.session = [AVCaptureSession new];
//    
//    NSError* error = nil;
//    AVCaptureDeviceInput* input = [AVCaptureDeviceInput deviceInputWithDevice:native_device error:&error];
//    if (!input) {
//        NSLog(@"open_session() error: %@", error);
//        return CAPTURE_ERROR;
//    }
//    
//    AVCaptureVideoDataOutput* output = [AVCaptureVideoDataOutput new];
//    native_session.queue = dispatch_queue_create(NULL, NULL);
//    [output setSampleBufferDelegate:native_session queue:native_session.queue];
//    
//    [native_session.session addInput:input];
//    [native_session.session addOutput:output];
//    
//    // TODO set delegate and figure out what to do with frames
//    // TODO remove this, maybe? Not sure if we do this here yet or externally.
//    [native_session.session startRunning];
//    
//    *session = CFBridgingRetain(native_session);
//    return CAPTURE_OK;
//}



/*

    OpenPnp-Capture: a video capture subsystem.

    Platform independent stream code

    Created by Niels Moseley on 7/11/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

*/

#include <memory.h> // for memcpy
#include "stream.h"
#include "context.h"


// **********************************************************************
//   Stream
// **********************************************************************

Stream::Stream() :
    m_owner(nullptr),
    m_isOpen(false),
    m_frames(0)
{
}

Stream::~Stream()
{
    LOG(LOG_DEBUG,"Stream::~Stream reports %d frames captured.\n", m_frames);
    //Note: close() should be called/handled by the PlatformStream!
}

bool Stream::hasNewFrame()
{
    m_bufferMutex.lock();
    bool ok = m_newFrame;
    m_bufferMutex.unlock();
    return ok;
}

bool Stream::captureFrame(uint8_t *RGBbufferPtr, uint32_t RGBbufferBytes)
{
    if (!m_isOpen) return false;

    m_bufferMutex.lock();    
    size_t maxBytes = RGBbufferBytes <= m_frameBuffer.size() ? RGBbufferBytes : m_frameBuffer.size();
    if (maxBytes != 0)
    {
        memcpy(RGBbufferPtr, &m_frameBuffer[0], maxBytes);
    }
    m_newFrame = false;
    m_bufferMutex.unlock();
    return true;
}

void Stream::submitBuffer(const uint8_t *ptr, size_t bytes)
{
    m_bufferMutex.lock();
    
    if (m_frameBuffer.size() == 0)
    {
        LOG(LOG_ERR,"Stream::m_frameBuffer size is 0 - cant store frame buffers!\n");
    }

    // Generate warning every 100 frames if the frame buffer is not
    // the expected size. 
    
    const uint32_t wantSize = m_width*m_height*3;
    if ((bytes != wantSize) && ((m_frames % 100) == 0))
    {
        LOG(LOG_WARNING, "Warning: captureFrame received incorrect buffer size (got %d want %d)\n", bytes, wantSize);
    }

    if (m_frameBuffer.size() >= bytes)
    {
        memcpy(&m_frameBuffer[0], ptr, bytes);
        m_newFrame = true; 
        m_frames++;
    }
    m_bufferMutex.unlock();
}

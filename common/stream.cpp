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
    close();
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

void Stream::submitBuffer(uint8_t *ptr, size_t bytes)
{
    m_bufferMutex.lock();
    if (m_frameBuffer.size() >= bytes)
    {
        memcpy(&m_frameBuffer[0], ptr, bytes);
        m_newFrame = true; 
        m_frames++;
    }
    m_bufferMutex.unlock();
}

/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Stream class

*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <string>

#include "platformdeviceinfo.h"
#include "platformstream.h"
#include "platformcontext.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

Stream* createPlatformStream()
{
    return new PlatformStream();
}

int xioctl(int fh, int request, void *arg)
{
    int r;

    do 
    {
        r = ioctl(fh, request, arg);
    } while ((r == -1) && (errno == EINTR));

    return r;
}

// **********************************************************************
//   Capture thread/function
// **********************************************************************

void captureThreadFunction(PlatformStream *stream, int fd, size_t bufferSizeBytes)
{
    if (stream == nullptr)
    {
        return;
    }

    LOG(LOG_DEBUG, "capture thread running (deviceHandle = %08X) ...\n", fd);

    // create local frame buffer
    std::vector<uint8_t> buffer(bufferSizeBytes);

    // FIXME: For now, weĺl just rely on the read to fail
    // when the PlatformStream closes the file
    // descriptor. This doesn't feel very professional,
    // but it should work :)
    while(!stream->getThreadQuitState())
    {
        ssize_t actualBytesRead = ::read(fd, &buffer[0], bufferSizeBytes);
        if (actualBytesRead < 0)
        {
            LOG(LOG_DEBUG, "capture thread exited (errno %d).\n", errno);
            return; //exit thread
        }

        // read will only return complete buffers
        stream->threadSubmitBuffer(&buffer[0], actualBytesRead);
        LOG(LOG_INFO, "yay\n");
    }
}

void captureThreadFunctionAsync(PlatformStream *stream, int fd, size_t bufferSizeBytes)
{
    //https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/capture.c.html
    const uint32_t nBuffers = 5;

    struct CaptureBufferInfo
    {
        uint8_t *start;
        size_t  length;
    };

    CaptureBufferInfo buffers[nBuffers];
    
    if (stream == nullptr)
    {
        return;
    }

    // ****************************************
    // create queue buffers
    // ****************************************

    v4l2_buf_type bufferType;
    for (uint32_t i = 0; i < nBuffers; ++i)
    {        
        v4l2_buffer   buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
        {
            LOG(LOG_ERR,"VIDIOC_QBUF failed (errno=%d)\n", errno);
        }
    }

    bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(fd, VIDIOC_STREAMON, &bufferType) == -1)
    {
        LOG(LOG_ERR,"VIDIOC_STREAMON failed (errno=%d)\n", errno);
    }

    while(!stream->getThreadQuitState())
    {
        fd_set fds;
        struct timeval tv;
        int result;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        result = select(fd + 1, &fds, NULL, NULL, &tv);
        if (result == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            LOG(LOG_ERR,"Select failed (errno=%d)\n", errno);
            return;
        }
        else if (result == 0)
        {
            LOG(LOG_ERR,"Select timeout\n");
            return;
        }

        // ****************************************
        // read the frame
        // ****************************************
        v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1)
        {
            switch (errno) 
            {
            case EAGAIN:
                return;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                LOG(LOG_ERR, "VIDIOC_DQBUF error\n");
                return;
            }
        }

        //assert(buf.index < nBuffers);
        stream->threadSubmitBuffer(buffers[buf.index].start, buf.bytesused);

        // re-queue the buffer
        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
        {
            LOG(LOG_ERR, "VIDIOC_DQBUF error\n");
            return;    
        }
    } // while  

    bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMOFF, &bufferType) == -1)
    {
        LOG(LOG_ERR, "VIDIOC_STREAMOFF error\n");
        return;     
    } 
}

// **********************************************************************
//   PlatformStream
// **********************************************************************

PlatformStream::PlatformStream() : 
    Stream(),
    m_quitThread(false),
    m_helperThread(nullptr)
{

}

PlatformStream::~PlatformStream()
{
    close();
}

void PlatformStream::close()
{
    LOG(LOG_INFO, "closing stream\n");

    m_owner = nullptr;
    m_width = 0;
    m_height = 0;
    m_frameBuffer.resize(0);
    m_isOpen = false; 
    m_quitThread = true;

    ::close(m_deviceHandle);

    if (m_helperThread != nullptr)
    {
        m_helperThread->join();
        
        delete m_helperThread;           
        
        m_helperThread = nullptr;
    }

    m_deviceHandle = -1;    
}

void test(size_t bufferSizeBytes)
{

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

    m_owner = owner;
    m_frames = 0;
    m_width = 0;
    m_height = 0;    

    //m_deviceHandle = ::open(dinfo->m_devicePath.c_str(), O_RDWR /* required */ | O_NONBLOCK);
    m_deviceHandle = ::open(dinfo->m_devicePath.c_str(), O_RDWR /* required */);
    if (m_deviceHandle < 0)
    {
        LOG(LOG_CRIT, "Could not open device %s (errno = %d)\n", dinfo->m_devicePath.c_str(), errno);
        close();
        return false;
    }

    m_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(m_deviceHandle, VIDIOC_G_FMT, &m_fmt) == -1)
    {
        LOG(LOG_CRIT, "Could not query default format (errno = %d)\n", errno);
        close();
        return false;
    }

    LOG(LOG_INFO, "Format buffer type: %d\n", m_fmt.type);
    if (m_fmt.type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
        LOG(LOG_ERR, "Buffer type (%d) not supported!\n", m_fmt.type);
        close();
        return false;
    }

    m_width = m_fmt.fmt.pix.width;
    m_height = m_fmt.fmt.pix.height;

    LOG(LOG_INFO, "Width  = %d pixels\n", m_fmt.fmt.pix.width);
    LOG(LOG_INFO, "Height = %d pixels\n", m_fmt.fmt.pix.height);
    LOG(LOG_INFO, "FOURCC = %s\n", genFOURCCstring(m_fmt.fmt.pix.pixelformat).c_str());

    m_isOpen = true;

    // create the helper thread to read from the device
    m_quitThread = false;

#if 0
    m_helperThread = new std::thread(&captureThreadFunction, this,
        m_deviceHandle, m_width*m_height*4);
#else
    m_helperThread = new std::thread(&captureThreadFunctionAsync, this,
        m_deviceHandle, m_width*m_height*4);
#endif
    return true;
}

uint32_t PlatformStream::getFOURCC()
{
    if (m_isOpen)
    {
        return m_fmt.fmt.pix.pixelformat;
    }
    else
    {
        return 0;
    }
}


bool PlatformStream::setExposure(int32_t value) 
{
    return false;
}


bool PlatformStream::setAutoExposure(bool enabled) 
{
    return false;
}


bool PlatformStream::getExposureLimits(int32_t *emin, int32_t *emax) 
{
    return false;
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


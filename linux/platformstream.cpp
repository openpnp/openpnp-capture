/*

    OpenPnp-Capture: a video capture subsystem.

    Linux platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Stream class

*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <memory.h>
#include <string>
#include "scopedptr.h"

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
//   PlatformStreamHelper functions
// **********************************************************************

bool PlatformStreamHelper::createAndMapBuffers(uint32_t nBuffers)
{
    v4l2_requestbuffers req;

    CLEAR(req);

    req.count  = nBuffers;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) 
    {
        LOG(LOG_ERR, "createAndMapBuffers failed - no memory mapping support.\n");
        return false;
    }

    if (req.count < 2) 
    {
        LOG(LOG_ERR, "createAndMapBuffers: need more than 1 buffer.\n");
        return false;
    }

    LOG(LOG_DEBUG, "Reserving %d mmap buffers\n", req.count);

    m_buffers.resize(req.count);

    for (uint32_t b = 0; b < req.count; ++b) 
    {
        v4l2_buffer buf;

        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = b;

        if (xioctl(m_fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            LOG(LOG_ERR, "createAndMapBuffers: VIDIOC_QUERYBUF failed.\n");
            return false;
        }

        m_buffers[b].length = buf.length;
        m_buffers[b].start  = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, 
            MAP_SHARED, m_fd, buf.m.offset);

        if (m_buffers[b].start == MAP_FAILED)
        {
            LOG(LOG_ERR, "createAndMapBuffers: mmap failed.\n");
            return false;
        }
        else
        {
            LOG(LOG_DEBUG, "Created mmap buffer of %d bytes\n", buf.length);
        }

    }

    return true;
}

void PlatformStreamHelper::unmapAndDeleteBuffers()
{
    for(uint32_t i=0; i<m_buffers.size(); i++)
    {
        munmap(m_buffers[i].start, m_buffers[i].length);
    }

    m_buffers.clear();
    LOG(LOG_DEBUG, "Mmap buffers deleted\n");
}

bool PlatformStreamHelper::queueAllBuffers()
{
    // ****************************************
    // create queue buffers
    // ****************************************

    v4l2_buf_type bufferType;
    for (uint32_t i = 0; i < m_buffers.size(); ++i)
    {        
        v4l2_buffer   buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(m_fd, VIDIOC_QBUF, &buf) == -1)
        {
            LOG(LOG_ERR,"VIDIOC_QBUF failed (errno=%d)\n", errno);
            return false;
        }
    }
    return true;
}

bool PlatformStreamHelper::streamOn()
{
    v4l2_buf_type bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(m_fd, VIDIOC_STREAMON, &bufferType) == -1)
    {
        LOG(LOG_ERR,"VIDIOC_STREAMON failed (errno=%d)\n", errno);
        return false;
    }

    LOG(LOG_DEBUG, "stream is On\n");

    return true;
}

bool PlatformStreamHelper::streamOff()
{
    v4l2_buf_type bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(m_fd, VIDIOC_STREAMOFF, &bufferType) == -1)
    {
        LOG(LOG_ERR,"VIDIOC_STREAMOFF failed (errno=%d)\n", errno);
        return false;
    }

    LOG(LOG_DEBUG, "stream is Off\n");

    return true;
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
    
    if (stream == nullptr)
    {
        return;
    }

    LOG(LOG_DEBUG, "captureThreadFunctionAsync started\n");

    PlatformStreamHelper *pHelper = new PlatformStreamHelper(fd);
    ScopedPtr<PlatformStreamHelper> helper(pHelper);

    if (!helper->createAndMapBuffers(nBuffers))
    {
        return;
    }
    
    if (!helper->queueAllBuffers())
    {
        return;
    }
    
    if (!helper->streamOn())
    {
        return;
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
                LOG(LOG_DEBUG, "VIDIOC_DQBUF returned EAGAIN\n");
                //FIXME: what to do here?!?
                continue;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                LOG(LOG_ERR, "VIDIOC_DQBUF error\n");
                return;
            }
        }

        //assert(buf.index < nBuffers);
        stream->threadSubmitBuffer(helper->getBufferPointer(buf.index), buf.bytesused);

        // re-queue the buffer
        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
        {
            LOG(LOG_ERR, "VIDIOC_DQBUF error\n");
            return;    
        }
    } // while  

    // Note: the destruction of the PlatformHelper 
    // by the scoped pointer will automaticall
    // turn off streaming and remove the
    // memory mapped buffers from the system.
    LOG(LOG_DEBUG, "captureThreadFunctionAsync exited\n");
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
    m_deviceHandle = ::open(dinfo->m_devicePath.c_str(), O_RDWR /* required */ | O_NONBLOCK);
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
    LOG(LOG_INFO, "FOURCC = %s\n", fourCCToString(m_fmt.fmt.pix.pixelformat).c_str());

    // set the (max) size of the frame buffer in Stream class
    m_frameBuffer.resize(m_width*m_height*4);

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


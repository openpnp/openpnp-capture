/*

    OpenPnp-Capture: a video capture subsystem.

    Linux context class to keep track of the global state.
    
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string>
#include <linux/videodev2.h>

#include "../common/logging.h"
#include "platformstream.h"
#include "platformcontext.h"

// a platform factory function needed by
// libmain.cpp
Context* createPlatformContext()
{
    return new PlatformContext();
}

PlatformContext::PlatformContext() :
    Context()
{
    LOG(LOG_DEBUG, "Context created\n");
    enumerateDevices();
}

PlatformContext::~PlatformContext()
{
}

bool PlatformContext::enumerateDevices()
{
    int fd;
    v4l2_capability  video_cap;

    LOG(LOG_INFO,"Enumerating devices\n");

    const uint32_t maxDevices = 64; // FIXME: is this a sane number for linux?

    uint32_t dcount = 0;
    while(dcount < maxDevices)
    {
        char fname[100];
        snprintf(fname, sizeof(fname), "/dev/video%d", dcount++);

        if ((fd = ::open(fname, O_RDWR /* required */ | O_NONBLOCK)) == -1)
        {
            //LOG(LOG_ERR, "enumerateDevices: Can't open device %s\n", fname);
            continue;
        }

        if (ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1)
        {
            ::close(fd);
            LOG(LOG_ERR, "enumerateDevices: Can't get capabilities\n");
            continue;
        }
        
        if ((video_cap.device_caps & V4L2_CAP_VIDEO_CAPTURE) != 0)
        {
            LOG(LOG_INFO,"Name: '%s'\n", video_cap.card);
            LOG(LOG_INFO,"Path: '%s'\n", fname);
            LOG(LOG_INFO,"Bus : '%s'\n", video_cap.bus_info);
            LOG(LOG_INFO,"capflags = %08X\n", video_cap.capabilities);
            LOG(LOG_INFO,"devflags = %08X\n", video_cap.device_caps);

            if ((video_cap.device_caps & V4L2_CAP_READWRITE) != 0)
            {
                LOG(LOG_INFO,"read/write supported\n");
            }
            else
            {
                LOG(LOG_INFO,"read/write NOT supported\n");
            }

            if ((video_cap.device_caps & V4L2_CAP_STREAMING) != 0)
            {
                LOG(LOG_INFO,"streaming I/O supported\n");
            }
            else
            {
                LOG(LOG_INFO,"streaming I/O NOT supported\n");
            }            

            if ((video_cap.device_caps & V4L2_CAP_ASYNCIO) != 0)
            {
                LOG(LOG_INFO,"async I/O supported\n");
            }
            else
            {
                LOG(LOG_INFO,"async I/O NOT supported\n");
            }   

            platformDeviceInfo* dinfo = new platformDeviceInfo();
            dinfo->m_name = std::string((const char*)video_cap.card);
            dinfo->m_devicePath = std::string(fname);
            dinfo->m_uniqueID = dinfo->m_name + " ";
            dinfo->m_uniqueID.append((const char*)video_cap.bus_info);
            
            // enumerate the frame formats
            v4l2_fmtdesc fmtdesc;
            uint32_t index = 0;
            fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            // FIXME: add FPS information
            // https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/vidioc-enum-frameintervals.html

            bool tryMore = true;
            while(tryMore)
            {
                fmtdesc.index = index;
            
                if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == -1)
                {
                    tryMore = false;
                }
                else
                {
                    LOG(LOG_INFO, "Format %d\n", index);
                    LOG(LOG_INFO, "  FOURCC = %s\n", fourCCToString(fmtdesc.pixelformat).c_str());

                    // .. then we enumerate all the frame buffer sizes for that
                    // pixel format type.
                    uint32_t frmindex = 0;
                    CapFormatInfo cinfo;
                    cinfo.fourcc = fmtdesc.pixelformat;
                    while(queryFrameSize(fd, frmindex, fmtdesc.pixelformat, &cinfo.width, &cinfo.height))
                    {
                        frmindex++;
                        cinfo.fps = findMaxFrameRate(fd, fmtdesc.pixelformat, cinfo.width, cinfo.height);
                        dinfo->m_formats.push_back(cinfo);
                        LOG(LOG_VERBOSE, "  %d x %d\n", cinfo.width, cinfo.height);
                    }
                }
                index++;
            }

            m_devices.push_back(dinfo);
        }

        ::close(fd);         
    }
    return true;
}


bool PlatformContext::queryFrameSize(int fd, uint32_t index, uint32_t pixelformat, uint32_t *width, uint32_t *height)
{
    v4l2_frmsizeenum frmSize;
    frmSize.index = index;
    frmSize.pixel_format = pixelformat;
    if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmSize) != -1)
    {
        if (frmSize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
        {
            *width  = frmSize.discrete.width;
            *height = frmSize.discrete.height;
        }
        else
        {
            LOG(LOG_WARNING, "queryFrameSize returned non-discrete frame size!\n");
            *width = 0;
            *height = 0;
        }

        return true;
    }
    return false;
}

uint32_t PlatformContext::findMaxFrameRate(int fd, uint32_t pixelformat, 
    uint32_t width, uint32_t height)
{
    uint32_t fps = 0;

    // now search the frame rates
    v4l2_frmivalenum ivals;
    ivals.pixel_format = pixelformat;
    ivals.width = width;
    ivals.height = height;
    ivals.index = 0;
    LOG(LOG_VERBOSE,"Finding max frame rates: \n");
    while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &ivals) != -1)
    {
        if (ivals.type == V4L2_FRMIVAL_TYPE_DISCRETE)
        {
            LOG(LOG_VERBOSE,"  FPS %d/%d\n", ivals.discrete.denominator, ivals.discrete.numerator);
            uint32_t v = ivals.discrete.denominator/ivals.discrete.numerator;
            if (fps < v)
            {
                fps = v;
            }
        }
        ivals.index++;
    }

    return fps;
}

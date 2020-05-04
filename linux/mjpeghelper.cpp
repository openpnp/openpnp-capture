/*

    OpenPnp-Capture: a video capture subsystem.

    Linux platform code, MJPEG decoding using libjpeg-turbo

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

#include "mjpeghelper.h"
#include "../common/logging.h"

bool MJPEGHelper::decompressFrame(const uint8_t *inBuffer,
    size_t inBytes, uint8_t *outBuffer,
    uint32_t outBufWidth, uint32_t outBufHeight)
{
    // note: the jpeg-turbo library apparently uses a non-const
    // buffer pointer to the incoming JPEG data.
    // Hopefully, the lib does not change the JPEG data buffer.
    // so, for now, we'll cast it to a non-const pointer.
    // Yes, that is completely dirty... I'm not happy with it either.

    uint8_t *jpegPtr = const_cast<uint8_t*>(inBuffer);
    int32_t width, height, jpegSubsamp;
    
    tjDecompressHeader2(m_decompressHandle, jpegPtr, inBytes, &width, &height, &jpegSubsamp);    
    if ((width != outBufWidth) || (height != outBufHeight))
    {
        LOG(LOG_ERR, "tjDecompressHeader2 failed: %s\n", tjGetErrorStr());
        return false;
    }
    else
    {
        LOG(LOG_VERBOSE, "MJPG: %d %d size %d bytes\n", width, height, inBytes);
    }

    if (tjDecompress2(m_decompressHandle, jpegPtr, inBytes, outBuffer, 
        width, 0/*pitch*/, height, TJPF_RGB, TJFLAG_FASTDCT) != 0)
    {
        // A lot of cameras produce incorrect but decodable JPEG data
        // and produce warnings that fill the console,
        // such as 'extraneous bytes before marker' etc.
        //
        // To avoid cluttering the console, we suppress the warnings
        // and errors completely.. :-/
        //
        // FIXME: the following disabled code only works for
        // very recent libjpeg-turbo libraries that aren't common on systems
        // yet..

        #if 0
        if (tjGetErrorCode(m_decompressHandle)==TJERR_ERROR)
        {
            LOG(LOG_ERR, "tjDecompress2 failed: %s\n", tjGetErrorStr());
            return false;
        }
        #endif

        return true;
    }

    return true;
}
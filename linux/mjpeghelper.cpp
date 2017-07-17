/*

    OpenPnp-Capture: a video capture subsystem.

    Linux platform code, MJPEG decoding using libjpeg-turbo

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

*/

#include "mjpeghelper.h"
#include "../common/logging.h"

bool MJPEGHelper::decompressFrame(const uint8_t *inBuffer,
    size_t inBytes, uint8_t *outBuffer,
    uint32_t outBufWidth, uint32_t outBufHeight)
{
    // note: the jpeg-turbo library apparently uses non-const
    // buffer pointer to the incoming JPEG data.
    // Hopefully, the lib does not change the JPEG data buffer.
    // so, for now, we'll cast it to a non-const pointer.
    // Yes, that is completely dirty... I'm not happy with it either.

    uint8_t *jpegPtr = const_cast<uint8_t*>(inBuffer);
    int32_t width, height, jpegSubsamp;
    
    tjDecompressHeader2(m_decompressHandle, jpegPtr, inBytes, &width, &height, &jpegSubsamp);    
    if ((width != outBufWidth) || (height != outBufHeight))
    {
        LOG(LOG_ERR, "tjDecompressHeader2 failed\n");
        return false;
    }

    if (tjDecompress2(m_decompressHandle, jpegPtr, inBytes, outBuffer, 
        width, 0/*pitch*/, height, TJPF_RGB, TJFLAG_FASTDCT) != 0)
    {
        LOG(LOG_ERR, "tjDecompress2 failed\n");
        return false;
    }

    return true;
}
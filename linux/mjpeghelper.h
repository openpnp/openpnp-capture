/*

    OpenPnp-Capture: a video capture subsystem.

    Linux platform code, MJPEG decoding using libjpeg-turbo

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

*/

#ifndef linux_mjpeghelper_h
#define linux_mjpeghelper_h

#include <turbojpeg.h>
#include <stdint.h>
#include <stdlib.h> // size_t

class MJPEGHelper
{
public:
    MJPEGHelper()
    {
        m_decompressHandle = tjInitDecompress();
    }

    virtual ~MJPEGHelper()
    {
        tjDestroy(m_decompressHandle);
    }

    /** Decompress a JPEG contained in the buffer. 
        The width and height of the output buffer are for
        sanity checking only. If the JPEG does not match
        the buffer size, the function will return false.
    */
    bool decompressFrame(const uint8_t *inBuffer, size_t inBytes, 
        uint8_t *outBuffer, uint32_t outBufWidth, uint32_t outButHeight);

protected:
    tjhandle m_decompressHandle;  ///< decompressor handle
};

#endif

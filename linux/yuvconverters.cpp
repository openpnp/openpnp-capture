/*

    OpenPnp-Capture: a video capture subsystem.

    Linux platform code

    Created by Niels Moseley on 9/21/17.
    Copyright © 2017 Niels Moseley, Jason von Nieda.     

    YUV to RGB conversion routines

*/

#include "yuvconverters.h"

static inline uint8_t clamp(int16_t v)
{
    v =  (v > 255) ? 255 : v;
    v =  (v < 0) ? 0 : v;
    return v;
}

/*
    In the YUYV2/YUV2 pixel, the order of the fields is:
    Y0 | Cr | Y1 | Cb ... repeating, which encode two 24-bit pixels.

    perfect:

    B = 1.164(Y - 16)                  + 2.018(U - 128)
    G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)
    R = 1.164(Y - 16) + 1.596(V - 128)

    or:

    R = Y + 1.403V'
    G = Y - 0.344U' - 0.714V'
    B = Y + 1.770U'

*/
void YUYV2RGB(const uint8_t *yuv, uint8_t *rgb, uint32_t bytes)
{
    while(bytes > 3)
    {
        int16_t y0 = *yuv++;    // Y0
        int16_t cr = *yuv++;    // Cr (aka U)
        int16_t y1 = *yuv++;    // Y1
        int16_t cb = *yuv++;    // Cb (aka V)

        int16_t yy0 = 19*(y0 - 16); 
        int16_t yy1 = 19*(y1 - 16); 
        *rgb++ = clamp((yy0                 + 32*(cb - 128)) >> 4);
        *rgb++ = clamp((yy0 - 13*(cr - 128) -  6*(cb - 128)) >> 4);
        *rgb++ = clamp((yy0 + 26*(cr - 128)                ) >> 4);
        *rgb++ = clamp((yy1                 + 32*(cb - 128)) >> 4);
        *rgb++ = clamp((yy1 - 13*(cr - 128) -  6*(cb - 128)) >> 4);
        *rgb++ = clamp((yy1 + 26*(cr - 128)                ) >> 4);
        bytes -= 4;
    }
}

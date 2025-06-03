/*

    OpenPnp-Capture: a video capture subsystem.

    Linux platform code
    YUV to RGB conversion routines

    Created by Niels Moseley on 9/21/17.
    Copyright (c) 2017 Jason von Nieda, Niels Moseley.

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

/*
    NV12 format has two planes:
    - Y plane: Full resolution luminance samples
    - UV plane: 2x2 subsampled interleaved U,V (Cb,Cr) samples

    Memory layout:
    YYYYYYYY
    YYYYYYYY
    YYYYYYYY
    YYYYYYYY
    UVUVUVUV
    UVUVUVUV

    Each 2x2 Y block shares one U,V pair.
*/
void NV122RGB(const uint8_t *nv12, uint8_t *rgb, uint32_t width, uint32_t height)
{
    const uint8_t *y_plane = nv12;
    const uint8_t *uv_plane = nv12 + (width * height);

    for (uint32_t row = 0; row < height; row++)
    {
        for (uint32_t col = 0; col < width; col++)
        {
            // Get Y value for current pixel
            int16_t y = y_plane[row * width + col];

            // Get U,V values (shared by 2x2 pixel blocks)
            uint32_t uv_row = row / 2;
            uint32_t uv_col = col / 2;
            uint32_t uv_index = uv_row * width + uv_col * 2;

            int16_t u = uv_plane[uv_index];      // Cb
            int16_t v = uv_plane[uv_index + 1];  // Cr

            // Convert YUV to RGB using the same coefficients as YUYV
            int16_t yy = 19 * (y - 16);

            // Calculate RGB index
            uint32_t rgb_index = (row * width + col) * 3;

            // R, G, B order (RGB24)
            rgb[rgb_index]     = clamp((yy + 26 * (v - 128)) >> 4);
            rgb[rgb_index + 1] = clamp((yy - 13 * (v - 128) - 6 * (u - 128)) >> 4);
            rgb[rgb_index + 2] = clamp((yy + 32 * (u - 128)) >> 4);
        }
    }
}

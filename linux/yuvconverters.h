/*

    OpenPnp-Capture: a video capture subsystem.

    Linux platform code

    Created by Niels Moseley on 9/21/17.
    Copyright © 2017 Niels Moseley, Jason von Nieda.     

    YUV to RGB conversion routines

*/

#ifndef linux_yuvconverters_h
#define linux_yuvconverters_h

#include <stdint.h>

void YUYV2RGB(const uint8_t *yuv, uint8_t *rgb, uint32_t bytes);

#endif
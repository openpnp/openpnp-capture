/*

    openpnp test application

    Niels Moseley

*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>

#include "openpnp-capture.h"
#include "../common/context.h"

int main(int argc, char*argv[])
{    
    printf("OpenPNP Capture Test Program\n");
    Cap_setLogLevel(7);

    CapContext ctx = Cap_createContext();

    uint32_t deviceCount = Cap_getDeviceCount(ctx);
    printf("Number of devices: %d\n", deviceCount);
    for(uint32_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
    {
        printf("Device [%d/%d] Name: %s\n", deviceIndex, deviceCount, Cap_getDeviceName(ctx, deviceIndex));
        printf("Unique ID: %s\n", Cap_getDeviceUniqueID(ctx, deviceIndex));
        uint32_t formatCount = Cap_getNumFormats(ctx, deviceIndex);
        printf("  Number of formats: %d\n", formatCount);
        for (uint32_t formatIndex = 0; formatIndex < formatCount; formatIndex++) {
            CapFormatInfo format;
            Cap_getFormatInfo(ctx, deviceIndex, formatIndex, &format);
            std::string fourcc;
            for (int i = 3; i >= 0; i--) {
                fourcc += (char) ((format.fourcc >> (8 * i)) & 0xff);
            }
            printf("  Format [%d/%d] %s %dx%d @ %d FPS\n", formatIndex, formatCount, fourcc.c_str(), format.width, format.height, format.fps);
        }
    }

//    int32_t streamID = Cap_openStream(ctx, 0, 0, 0, 0);
//    printf("Stream ID = %d\n", streamID);
//    
//    if (Cap_isOpenStream(ctx, streamID) == 1)
//    {
//        printf("Stream is open\n");
//    }
//    else
//    {
//        printf("Stream is closed (?)\n");
//    }
//
//    printf("Press Q to exit..\n");
//
//    std::vector<uint8_t> m_buffer;
//    m_buffer.resize(640*480*3);
//
//    Cap_setAutoExposure(ctx, streamID, 1);
//
//#if 1
//    uint32_t counter = 0;
//    uint32_t tries = 0;
//    while(counter < 30)
//    {
//        usleep(50000);
//        printf("%d", Cap_getStreamFrameCount(ctx, streamID));
//        if (Cap_hasNewFrame(ctx, streamID) == 1)
//        {
//            Cap_captureFrame(ctx, streamID, &m_buffer[0], m_buffer.size());
//            counter++;
//        }
//        tries++;
//        if (tries == 1000)
//        {
//            break;
//        }
//    };
//#endif
//
//    Cap_setAutoExposure(ctx, streamID, 0);
//
//    // wait for a new frame .. 
//    //while (Cap_hasNewFrame(ctx, streamID) == 0) {};
//    
//    if (Cap_captureFrame(ctx, streamID, &m_buffer[0], m_buffer.size()) == CAPRESULT_OK)
//    {
//        printf("Buffer captured!\n");
//
//        FILE *fout = fopen("image.ppm", "wb");
//        fprintf(fout, "P6 640 480 255\n"); // PGM header
//
//        const uint32_t height = 480;
//        const uint32_t width  = 640;
//
//        // exchange BGR to RGB
//        uint32_t idx = 0;
//        for(uint32_t i=0; i<width*height; i++)
//        {
//            uint8_t b = m_buffer[idx];
//            uint8_t g = m_buffer[idx+1];
//            uint8_t r = m_buffer[idx+2];
//            m_buffer[idx++] = r;
//            m_buffer[idx++] = g;
//            m_buffer[idx++] = b;
//        }
//
//        // and upside-down :)
//        const uint32_t stride = 3;
//        const size_t lineBytes = width * stride;
//        uint8_t *row  = new uint8_t[lineBytes];
//        uint8_t *low  = &m_buffer[0];
//        uint8_t *high = &m_buffer[(height - 1) * lineBytes];
//
//        for (; low < high; low += lineBytes, high -= lineBytes) {
//            memcpy(row, low, lineBytes);
//            memcpy(low, high, lineBytes);
//            memcpy(high, row, lineBytes);
//        }
//        delete[] row;
//
//        fwrite(&m_buffer[0], 1, m_buffer.size(), fout);
//        fclose(fout);
//    }
//
//    char c = 0;
//    int32_t v = 0;
//    while((c != 'q') && (c != 'Q'))
//    {
//        c = getchar();
//        switch(c)
//        {
//        case '+':
//            printf("+");
//            Cap_setExposure(ctx, streamID, ++v);
//            break;
//        case '-':
//            printf("-");
//            Cap_setExposure(ctx, streamID, --v);
//            break;
//        case '0':
//            printf("0");
//            v = 0;
//            Cap_setExposure(ctx, streamID, v);
//            break;        
//        }
//    }
//
//    Cap_closeStream(ctx, streamID);

    CapResult result = Cap_releaseContext(ctx);

    return 0;
}


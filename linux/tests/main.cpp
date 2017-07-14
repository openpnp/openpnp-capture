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

std::string FourCCToString(uint32_t fourcc)
{
    std::string v;
    for(uint32_t i=0; i<4; i++)
    {
        v += static_cast<char>(fourcc & 0xFF);
        fourcc >>= 8;
    }
    return v;
}

int main(int argc, char*argv[])
{    
    uint32_t deviceFormatID = 0;
    uint32_t deviceID       = 0;

    printf("OpenPNP Capture Test Program\n");
    Cap_setLogLevel(7);

    if (argc >= 2)
    {
        deviceID = atoi(argv[1]);
    }
    
    if (argc >= 3)
    {
        deviceFormatID = atoi(argv[2]);
    }

    CapContext ctx = Cap_createContext();

    uint32_t deviceCount = Cap_getDeviceCount(ctx);
    printf("Number of devices: %d\n", deviceCount);
    for(uint32_t i=0; i<deviceCount; i++)
    {
        printf("ID %d -> %s\n", i, Cap_getDeviceName(ctx,i));

        // show all supported frame buffer formats
        int32_t nFormats = Cap_getNumFormats(ctx, i);

        printf("  Number of formats: %d\n", nFormats);

        std::string fourccString;
        for(int32_t j=0; j<nFormats; j++)
        {
            CapFormatInfo finfo;
            Cap_getFormatInfo(ctx, i, j, &finfo);
            fourccString = FourCCToString(finfo.fourcc);

            printf("  Format ID %d: %d x %d pixels  FOURCC=%s\n",
                j, finfo.width, finfo.height, fourccString.c_str());
        }
    }

    int32_t streamID = Cap_openStream(ctx, deviceID, deviceFormatID);
    printf("Stream ID = %d\n", streamID);
    
    if (Cap_isOpenStream(ctx, streamID) == 1)
    {
        printf("Stream is open\n");
    }
    else
    {
        printf("Stream is closed (?)\n");
        return 1;
    }

    printf("Press Q to exit..\n");

    // get current stream parameters 
    CapFormatInfo finfo;
    Cap_getFormatInfo(ctx, deviceID, deviceFormatID, &finfo);

    Cap_setAutoExposure(ctx, streamID, 0);

#if 1
    std::vector<uint8_t> m_buffer;
    m_buffer.resize(finfo.width*finfo.height*4);

    uint32_t counter = 0;
    uint32_t tries = 0;
    while(counter < 30)
    {
        usleep(50*1000);
        printf("%d", Cap_getStreamFrameCount(ctx, streamID));
        if (Cap_hasNewFrame(ctx, streamID) == 1)
        {
            Cap_captureFrame(ctx, streamID, &m_buffer[0], m_buffer.size());

            // write frames to disk
            char fname[100];
            sprintf(fname, "frame%d.dat", counter);
            FILE *fout = fopen(fname, "wb");
            fwrite(&m_buffer[0], 1, m_buffer.size(), fout);
            fclose(fout);

            counter++;
        }
        tries++;
        if (tries == 1000)
        {
            break;
        }
    };
#endif

#if 0
    Cap_setAutoExposure(ctx, streamID, 0);

    // wait for a new frame .. 
    //while (Cap_hasNewFrame(ctx, streamID) == 0) {};
    
    if (Cap_captureFrame(ctx, streamID, &m_buffer[0], m_buffer.size()) == CAPRESULT_OK)
    {
        printf("Buffer captured!\n");

        FILE *fout = fopen("image.ppm", "wb");
        fprintf(fout, "P6 %d %d 255\n", finfo.width, finfo.height); // PGM header

        // exchange BGR to RGB
        uint32_t idx = 0;
        for(uint32_t i=0; i<finfo.width*finfo.height; i++)
        {
            uint8_t b = m_buffer[idx];
            uint8_t g = m_buffer[idx+1];
            uint8_t r = m_buffer[idx+2];
            m_buffer[idx++] = r;
            m_buffer[idx++] = g;
            m_buffer[idx++] = b;
        }

        // and upside-down :)
        const uint32_t stride = 3;
        const size_t lineBytes = finfo.width * stride;
        uint8_t *row  = new uint8_t[lineBytes];
        uint8_t *low  = &m_buffer[0];
        uint8_t *high = &m_buffer[(finfo.height - 1) * lineBytes];

        for (; low < high; low += lineBytes, high -= lineBytes) {
            memcpy(row, low, lineBytes);
            memcpy(low, high, lineBytes);
            memcpy(high, row, lineBytes);
        }
        delete[] row;

        fwrite(&m_buffer[0], 1, m_buffer.size(), fout);
        fclose(fout);
    }

#endif

    char c = 0;
    int32_t v = 0;
    while((c != 'q') && (c != 'Q'))
    {
        c = getchar();
        switch(c)
        {
        case '+':
            printf("+");
            Cap_setExposure(ctx, streamID, ++v);
            break;
        case '-':
            printf("-");
            Cap_setExposure(ctx, streamID, --v);
            break;
        case '0':
            printf("0");
            v = 0;
            Cap_setExposure(ctx, streamID, v);
            break;        
        }
    }

    Cap_closeStream(ctx, streamID);

    CapResult result = Cap_releaseContext(ctx);

    return 0;
}


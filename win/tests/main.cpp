/*

    openpnp test application

    Niels Moseley

*/
#include <stdio.h>
#include <conio.h>
#include <windows.h> // for Sleep

#include "openpnp-capture.h"
#include "context.h"

int main(int argc, char*argv[])
{    
    printf("OpenPNP Capture Test Program\n");
    Cap_setLogLevel(7);

    CapContext ctx = Cap_createContext();

    uint32_t deviceCount = Cap_getDeviceCount(ctx);
    printf("Number of devices: %d\n", deviceCount);
    for(uint32_t i=0; i<deviceCount; i++)
    {
        printf("ID %d -> %s\n", i, Cap_getDeviceName(ctx,i));
    }

    int32_t streamID = Cap_openStream(ctx, 0, 0, 0, 0);
    printf("Stream ID = %d\n", streamID);
    
    if (Cap_isOpenStream(ctx, streamID) == 1)
    {
        printf("Stream is open\n");
    }
    else
    {
        printf("Stream is closed (?)\n");
    }

    printf("Press any key to exit..\n");

    std::vector<uint8_t> m_buffer;
    m_buffer.resize(640*480*3);

#if 1
    uint32_t counter = 0;
    uint32_t tries = 0;
    while(counter < 50)
    {
        Sleep(50);
        printf("%d", Cap_getStreamFrameCount(ctx, streamID));
        if (Cap_hasNewFrame(ctx, streamID) == 1)
        {
            Cap_captureFrame(ctx, streamID, &m_buffer[0], m_buffer.size());
            counter++;
        }
        tries++;
        if (tries == 1000)
        {
            break;
        }
    };
#endif

    // wait for a new frame .. 
    //while (Cap_hasNewFrame(ctx, streamID) == 0) {};
    
    if (Cap_captureFrame(ctx, streamID, &m_buffer[0], m_buffer.size()) == CAPRESULT_OK)
    {
        printf("Buffer captured!\n");

#if 0
A "magic number" for identifying the file type. A ppm image's magic number is the two characters "P6".
Whitespace (blanks, TABs, CRs, LFs).
A width, formatted as ASCII characters in decimal.
Whitespace.
A height, again in ASCII decimal.
Whitespace.
The maximum color value (Maxval), again in ASCII decimal. Must be less than 65536 and more than zero.
A single whitespace character (usually a newline).
A raster of Height rows, in order from top to bottom. Each row consists of Width pixels, in order from left to right. Each pixel is a triplet of red, green, and blue samples, in that order. Each sample is represented in pure binary by either 1 or 2 bytes. If the Maxval is less than 256, it is 1 byte. Otherwise, it is 2 bytes. The most significant byte is first. 
#endif

        FILE *fout = fopen("image.ppm", "wb");
        fprintf(fout, "P6 640 480 255\n"); // PGM header

        const uint32_t height = 480;
        const uint32_t width  = 640;

        // exchange BGR to RGB
        uint32_t idx = 0;
        for(uint32_t i=0; i<width*height; i++)
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
        const size_t lineBytes = width * stride;
        uint8_t *row  = new uint8_t[lineBytes];
        uint8_t *low  = &m_buffer[0];
        uint8_t *high = &m_buffer[(height - 1) * lineBytes];

        for (; low < high; low += lineBytes, high -= lineBytes) {
            memcpy(row, low, lineBytes);
            memcpy(low, high, lineBytes);
            memcpy(high, row, lineBytes);
        }
        delete[] row;

        fwrite(&m_buffer[0], 1, m_buffer.size(), fout);
        fclose(fout);
    }

    getch();

    Cap_closeStream(ctx, streamID);

    CapResult result = Cap_releaseContext(ctx);

    return 0;
}


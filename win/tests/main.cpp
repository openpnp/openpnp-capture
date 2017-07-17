/*

    openpnp test application

    Niels Moseley

*/
#include <stdio.h>
#include <conio.h>
#include <windows.h> // for Sleep

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

bool writeBufferAsPPM(uint32_t frameNum, uint32_t width, uint32_t height, const uint8_t *bufferPtr, size_t bytes)
{
    char fname[100];
    sprintf(fname, "frame_%d.ppm",frameNum);
    
    FILE *fout = fopen(fname, "wb");
    if (fout == 0)
    {
        fprintf(stderr, "Cannot open %s for writing\n", fname);
        return false;
    }

    fprintf(fout, "P6 %d %d 255\n", width, height); // PGM header
    fwrite(bufferPtr, 1, bytes, fout);
    fclose(fout);

    return true;
}

int main(int argc, char*argv[])
{    
    uint32_t deviceFormatID = 0;
    uint32_t deviceID       = 0;

    printf("OpenPNP Capture Test Program\n");
    Cap_setLogLevel(7);

    if (argc == 1)
    {
        printf("Usage: openpnp-capture-test <camera ID> <frame format ID>\n");
        printf("\n..continuing with default camera parameters.\n\n");
    }

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
        Cap_releaseContext(ctx);
        return 1;
    }

    printf("Press Q to exit.\n");
    printf("Press + or - to change the exposure.\n");
    printf("Press w to write the current frame to a PPM file.\n");

    // get current stream parameters 
    CapFormatInfo finfo;
    Cap_getFormatInfo(ctx, deviceID, deviceFormatID, &finfo);

    Cap_setAutoExposure(ctx, streamID, 0);

    // try to create a message loop so the preview
    // window doesn't crash.. 

    MSG msg;
    BOOL bRet;

    std::vector<uint8_t> m_buffer;
    m_buffer.resize(finfo.width*finfo.height*3);

    char c = 0;
    int32_t v = 0; // exposure value
    uint32_t frameWriteCounter=0;
    while((c != 'q') && (c != 'Q'))
    {
        if (PeekMessage(&msg, NULL, 0, 0, 0) != 0)
        {
            bRet = GetMessage(&msg, NULL, 0, 0);

            if (bRet > 0)  // (bRet > 0 indicates a message that must be processed.)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (_kbhit())
        {
            c = _getch();
            switch(c)
            {
            case '+':                
                Cap_setExposure(ctx, streamID, ++v);
                printf("exposure = %d  \r", v);
                break;
            case '-':
                printf("-");
                Cap_setExposure(ctx, streamID, --v);
                printf("exposure = %d  \r", v);
                break;
            case '0':
                printf("0");
                v = 0;
                Cap_setExposure(ctx, streamID, v);
                printf("exposure = %d  \r", v);
                break;
            case 'w':
                if (Cap_captureFrame(ctx, streamID, &m_buffer[0], m_buffer.size()) == CAPRESULT_OK)
                {
                    if (writeBufferAsPPM(frameWriteCounter, 
                        finfo.width,
                        finfo.height,
                        &m_buffer[0],
                        m_buffer.size()))
                    {
                        printf("Written frame to frame_%d.ppm\n", frameWriteCounter++);
                    }
                }
                break;
            }            
        }
        Sleep(10);
    }

    Cap_closeStream(ctx, streamID);

    CapResult result = Cap_releaseContext(ctx);

    return 0;
}


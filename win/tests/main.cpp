/*

    openpnp test application

    Niels Moseley

*/
#include <stdio.h>
#include <conio.h>
#include <windows.h> // for Sleep
#include <chrono>    

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


void estimateFrameRate(CapContext ctx, int32_t streamID)
{
    std::chrono::time_point<std::chrono::system_clock> tstart, tend;
    tstart = std::chrono::system_clock::now();
    uint32_t fstart = Cap_getStreamFrameCount(ctx, streamID);
    Sleep(2000);    // 2-second wait
    uint32_t fend = Cap_getStreamFrameCount(ctx, streamID);
    tend = std::chrono::system_clock::now();
    std::chrono::duration<double> fsec = tend-tstart;
    uint32_t frames = fend - fstart;
    printf("Frames = %d\n", frames);
    std::chrono::milliseconds d = std::chrono::duration_cast<std::chrono::milliseconds>(fsec);
    printf("Measured fps=%5.2f\n", 1000.0f*frames/static_cast<float>(d.count()));
}   

int main(int argc, char*argv[])
{    
    uint32_t deviceFormatID = 0;
    uint32_t deviceID       = 0;

    printf("==============================\n");
    printf(" OpenPNP Capture Test Program\n");
    printf(" %s\n", Cap_getLibraryVersion());
    printf("==============================\n");
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

    printf("=== KEY MAPPINGS ===\n");
    printf("Press q to exit.\n");
    printf("Press + or - to change the exposure.\n");
    printf("Press f or g to change the focus.\n");
    printf("Press z or x to change the zoom.\n");
    printf("Press a or s to change the gain.\n");
    printf("Press [ or ] to change the white balance.\n");
    printf("Press p to estimate the actual frame rate.\n");
    printf("Press w to write the current frame to a PPM file.\n");

    // get current stream parameters 
    CapFormatInfo finfo;
    Cap_getFormatInfo(ctx, deviceID, deviceFormatID, &finfo);

    //disable auto exposure, focus and white balance
    Cap_setAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE, 0);
    Cap_setAutoProperty(ctx, streamID, CAPPROPID_FOCUS, 0);
    Cap_setAutoProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, 0);
    Cap_setAutoProperty(ctx, streamID, CAPPROPID_GAIN, 0);

    // set exposure in the middle of the range
    int32_t exposure = 0;
    int32_t exmax, exmin;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_EXPOSURE, &exmin, &exmax) == CAPRESULT_OK)
    {
        exposure = (exmax + exmin) / 2;
        Cap_setProperty(ctx, streamID, CAPPROPID_EXPOSURE, exposure);
        printf("Set exposure to %d\n", exposure);
    }
    else
    {
        printf("Could not get exposure limits.\n");
    }

    // set focus in the middle of the range
    int32_t focus = 0;
    int32_t fomax, fomin;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_FOCUS, &fomin, &fomax) == CAPRESULT_OK)
    {
        focus = (fomax + fomin) / 2;
        Cap_setProperty(ctx, streamID, CAPPROPID_FOCUS, focus);
        printf("Set focus to %d\n", focus);
    }
    else
    {
        printf("Could not get focus limits.\n");
    }

    // set zoom in the middle of the range
    int32_t zoom = 0;
    int32_t zomax, zomin;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_ZOOM, &zomin, &zomax) == CAPRESULT_OK)
    {
        zoom = zomin;
        Cap_setProperty(ctx, streamID, CAPPROPID_ZOOM, zoom);
        printf("Set zoom to %d\n", zoom);
    }
    else
    {
        printf("Could not get zoom limits.\n");
    }

    // set white balance in the middle of the range
    int32_t wbalance = 0;
    int32_t wbmax, wbmin;
    int32_t wbstep = 0;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_WHITEBALANCE, &wbmin, &wbmax) == CAPRESULT_OK)
    {
        wbalance = (wbmax+wbmin)/2;
        wbstep = (wbmax-wbmin) / 20;
        Cap_setProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, wbalance);
        printf("Set white balance to %d\n", wbalance);
    }
    else
    {
        printf("Could not get white balance limits.\n");
    }

    // set gain in the middle of the range
    int32_t gain = 0;
    int32_t gmax, gmin;
    int32_t gstep = 0;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_GAIN, &gmin, &gmax) == CAPRESULT_OK)
    {
        gstep = (gmax-gmin) / 20;
        Cap_setProperty(ctx, streamID, CAPPROPID_GAIN, gain);
        printf("Set gain to %d (min=%d max=%d)\n", gain, gmin, gmax);
    }
    else
    {
        printf("Could not get gain limits.\n");
    }

    // try to create a message loop so the preview
    // window doesn't crash.. 

    MSG msg;
    BOOL bRet;

    std::vector<uint8_t> m_buffer;
    m_buffer.resize(finfo.width*finfo.height*3);

    char c = 0;
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
                Cap_setProperty(ctx, streamID, CAPPROPID_EXPOSURE, ++exposure);
                printf("exposure = %d  \r", exposure);
                break;
            case '-':
                Cap_setProperty(ctx, streamID, CAPPROPID_EXPOSURE, --exposure);
                printf("exposure = %d  \r", exposure);
                break;
            case '0':
                exposure = (exmax + exmin) / 2;
                Cap_setProperty(ctx, streamID, CAPPROPID_EXPOSURE, exposure);
                printf("exposure = %d  \r", exposure);
                break;
            case 'f':
                Cap_setProperty(ctx, streamID, CAPPROPID_FOCUS, ++focus);
                printf("focus = %d     \r", focus);
                break;
            case 'g':
                Cap_setProperty(ctx, streamID, CAPPROPID_FOCUS, --focus);
                printf("focus = %d     \r", focus);
                break;
            case 'z':
                Cap_setProperty(ctx, streamID, CAPPROPID_ZOOM, ++zoom);
                printf("zoom = %d     \r", zoom);
                break;
            case 'x':
                Cap_setProperty(ctx, streamID, CAPPROPID_ZOOM, --zoom);
                printf("zoom = %d     \r", zoom);
                break;  
            case '[':
                wbalance -= wbstep;
                Cap_setProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, wbalance);
                printf("wbal = %d     \r", wbalance);
                break;              
            case ']':
                wbalance += wbstep; 
                Cap_setProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, wbalance);
                printf("wbal = %d     \r", wbalance);
                break;  
            case 'a':
                gain -= gstep;
                Cap_setProperty(ctx, streamID, CAPPROPID_GAIN, gain);
                printf("gain = %d     \r", gain);
                break;              
            case 's':
                gain += gstep;
                Cap_setProperty(ctx, streamID, CAPPROPID_GAIN, gain);
                printf("gain = %d     \r", gain);
                break;
            case 'p':
                printf("Estimating frame rate..\n");
                estimateFrameRate(ctx, streamID);
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



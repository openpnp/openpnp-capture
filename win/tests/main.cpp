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

void myCustomLogFunction(uint32_t level, const char *string)
{
    printf("== %s", string);
}

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

void showAutoProperty(CapContext ctx, int32_t streamID, uint32_t propertyID)
{
    uint32_t bValue;
    if (Cap_getAutoProperty(ctx, streamID, propertyID, &bValue)==CAPRESULT_OK)
    {
        if (bValue) 
        {
            printf("Auto\n");
        }
        else
        {
            printf("Manual\n");
        }
    }
    else
    {
        printf("Unsupported\n");
    }
}

void showAutoProperties(CapContext ctx, int32_t streamID)
{
    printf("White balance: ");
    showAutoProperty(ctx, streamID, CAPPROPID_WHITEBALANCE);

    printf("Exposure     : ");
    showAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE);

    printf("Focus        : ");
    showAutoProperty(ctx, streamID, CAPPROPID_FOCUS);
    
    printf("Zoom         : ");
    showAutoProperty(ctx, streamID, CAPPROPID_ZOOM);

    printf("Gain         : ");
    showAutoProperty(ctx, streamID, CAPPROPID_GAIN);
}

void showProperty(CapContext ctx, int32_t streamID, uint32_t propertyID)
{
    int32_t value;
    if (Cap_getProperty(ctx, streamID, propertyID, &value)==CAPRESULT_OK)
    {
        printf("%d\n", value);
    }
    else
    {
        printf("Unsupported\n");
    }
}

void showProperties(CapContext ctx, int32_t streamID)
{
    printf("White balance: ");
    showProperty(ctx, streamID, CAPPROPID_WHITEBALANCE);

    printf("Exposure     : ");
    showProperty(ctx, streamID, CAPPROPID_EXPOSURE);

    printf("Focus        : ");
    showProperty(ctx, streamID, CAPPROPID_FOCUS);
    
    printf("Zoom         : ");
    showProperty(ctx, streamID, CAPPROPID_ZOOM);

    printf("Gain         : ");
    showProperty(ctx, streamID, CAPPROPID_GAIN);

    printf("Brightness   : ");
    showProperty(ctx, streamID, CAPPROPID_BRIGHTNESS);
    
    printf("Contrast     : ");
    showProperty(ctx, streamID, CAPPROPID_CONTRAST);
    
    printf("Saturation   : ");
    showProperty(ctx, streamID, CAPPROPID_SATURATION);
    
    printf("Gamma        : ");
    showProperty(ctx, streamID, CAPPROPID_GAMMA);
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

    Cap_installCustomLogFunction(myCustomLogFunction);

    printf("==============================\n");
    printf(" OpenPNP Capture Test Program\n");
    printf(" %s\n", Cap_getLibraryVersion());
    printf("==============================\n");
    Cap_setLogLevel(8);

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
        printf("Unique:  %s\n", Cap_getDeviceUniqueID(ctx,i));

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

    printf("Camera set to:\n");
    showProperties(ctx, streamID);
    showAutoProperties(ctx, streamID);
    
    printf("=== KEY MAPPINGS ===\n");
    printf("Press q to exit.\n");
    printf("Press + or - to change the exposure.\n");
    printf("Press 1 or 2 to change to auto/manual exposure.\n");
    printf("Press f or g to change the focus.\n");
    printf("Press z or x to change the zoom.\n");
    printf("Press a or s to change the gain.\n");
    printf("Press [ or ] to change the white balance.\n");
    printf("Press d to display the camera configuration.\n");
    printf("Press p to estimate the actual frame rate.\n");
    printf("Press w to write the current frame to a PPM file.\n");

    // get current stream parameters 
    CapFormatInfo finfo;
    Cap_getFormatInfo(ctx, deviceID, deviceFormatID, &finfo);

    //disable auto exposure, focus and white balance
    if (Cap_setAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE, 0) != CAPRESULT_OK)
    {
        printf("Could not disable auto-exposure\n");
    }

    if (Cap_setAutoProperty(ctx, streamID, CAPPROPID_FOCUS, 0) != CAPRESULT_OK)
    {
        printf("Could not disable auto-focus\n");
    }

    if (Cap_setAutoProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, 0) != CAPRESULT_OK)
    {
        printf("Could not disabe auto-whitebalance\n");
    }

    if (Cap_setAutoProperty(ctx, streamID, CAPPROPID_GAIN, 0) != CAPRESULT_OK)
    {
        printf("Could not disable auto-gain\n");
    }

    // set exposure in the middle of the range
    int32_t exposure = 0;
    int32_t exmax, exmin, edefault;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_EXPOSURE, &exmin, &exmax, &edefault) == CAPRESULT_OK)
    {
        //exposure = (exmax + exmin) / 2;
        exposure = edefault;
        Cap_setProperty(ctx, streamID, CAPPROPID_EXPOSURE, exposure);
        printf("Set exposure to %d\n", exposure);
        printf("Default exposure is : %d\n", edefault);
    }
    else
    {
        printf("Could not get exposure limits.\n");
    }

    // set focus in the middle of the range
    int32_t focus = 0;
    int32_t fomax, fomin, fodefault;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_FOCUS, &fomin, &fomax, &fodefault) == CAPRESULT_OK)
    {
        focus = (fomax + fomin) / 2;
        Cap_setProperty(ctx, streamID, CAPPROPID_FOCUS, focus);
        printf("Set focus to %d\n", focus);
        printf("Default focus is : %d\n", fodefault);
    }
    else
    {
        printf("Could not get focus limits.\n");
    }

    // set zoom in the middle of the range
    int32_t zoom = 0;
    int32_t zomax, zomin, zodefault;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_ZOOM, &zomin, &zomax, &zodefault) == CAPRESULT_OK)
    {
        zoom = zomin;
        Cap_setProperty(ctx, streamID, CAPPROPID_ZOOM, zoom);
        printf("Set zoom to %d\n", zoom);
        printf("Default zoom is : %d\n", zodefault);
    }
    else
    {
        printf("Could not get zoom limits.\n");
    }

    // set white balance in the middle of the range
    int32_t wbalance = 0;
    int32_t wbmax, wbmin, wbdefault;
    int32_t wbstep = 0;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_WHITEBALANCE, &wbmin, &wbmax, &wbdefault) == CAPRESULT_OK)
    {
        wbalance = (wbmax+wbmin)/2;
        wbstep = (wbmax-wbmin) / 20;
        Cap_setProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, wbalance);
        printf("Set white balance to %d\n", wbalance);
        printf("Default white balance is : %d\n", wbdefault);
    }
    else
    {
        printf("Could not get white balance limits.\n");
    }

    // set gain in the middle of the range
    int32_t gain = 0;
    int32_t gmax, gmin, gdefault;
    int32_t gstep = 0;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_GAIN, &gmin, &gmax, &gdefault) == CAPRESULT_OK)
    {
        gstep = (gmax-gmin) / 20;
        Cap_setProperty(ctx, streamID, CAPPROPID_GAIN, gain);
        printf("Set gain to %d (min=%d max=%d)\n", gain, gmin, gmax);
        printf("Default gain is : %d\n", gdefault);
    }
    else
    {
        printf("Could not get gain limits.\n");
    }


    printf("Camera reconfigured to:\n");
    showProperties(ctx, streamID);
    showAutoProperties(ctx, streamID);

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
            case '1':
                Cap_setAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE, 1);
                printf("exposure = auto  \r");
                break;
            case '2':
                Cap_setAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE, 0);
                printf("exposure = manual  \r");
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
            case 'd':
                printf("\nCamera configuration:\n");
                showProperties(ctx, streamID);
                showAutoProperties(ctx, streamID);
                printf("\n");
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



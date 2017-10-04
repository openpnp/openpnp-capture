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
#include "../uvcctrl.h"

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

#if 0    
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

#else

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
            //fourccString = FourCCToString(finfo.fourcc);
            std::string fourcc;
            for (int i = 3; i >= 0; i--) {
                fourcc += (char) ((finfo.fourcc >> (8 * i)) & 0xff);
            }
            printf("  Format ID %d: %d x %d pixels  %d FPS(max)  FOURCC=%s\n",
                j, finfo.width, finfo.height, finfo.fps, fourcc.c_str());
        }
    }

#endif 

    // get current stream parameters 
    CapFormatInfo finfo;
    Cap_getFormatInfo(ctx, deviceID, deviceFormatID, &finfo);


    #if 0
    UVCCtrl *ctrl = UVCCtrl::create(0x05AC, 0x8502);
    if (ctrl != nullptr)
    {
        printf("Created UVC control interface!\n");

        bool state;
        if (ctrl->getAutoProperty(CAPPROPID_WHITEBALANCE, state))
        {
            printf("Auto white balance is: %s\n", state ? "ON" : " OFF");
        }
        else
        {
            printf("Cannot get white balance state..\n");
        }
        
        int32_t value, emin, emax;
        if (ctrl->getProperty(CAPPROPID_EXPOSURE, &value))
        {
            printf("Exposure is: %d\n", value);
        }
        else
        {
            printf("Cannot get exposure..\n");
        }

        if (ctrl->getPropertyLimits(CAPPROPID_EXPOSURE, &emin, &emax))
        {
            printf("Exposure limits are: %d .. %d\n", emin, emax);
        }
        else
        {
            printf("Cannot get exposure limits..\n");
        }

        delete ctrl;
    }
    else
    {
        printf("Failed to create UVC control interface\n");
    }
    #endif
    
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

    int32_t emin,emax,edefault;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_EXPOSURE, &emin, &emax, &edefault) == CAPRESULT_OK)
    {
        printf("Exposure limits: %d .. %d (default=%d)\n", emin, emax, edefault);
    }
    else
    {
        printf("Failed to get exposure limits!\n");
    }

    if (Cap_getProperty(ctx, streamID, CAPPROPID_EXPOSURE, &emin) == CAPRESULT_OK)
    {
        printf("Exposure: %d\n", emin);
    }
    else
    {
        printf("Failed to get exposure!\n");
    }

    uint32_t vvv;
    if (Cap_getAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE, &vvv) == CAPRESULT_OK)
    {
        printf("Auto exposure: %d\n", vvv);
    }
    else
    {
        printf("Failed to get auto exposure!\n");
    }

    //disable auto exposure, focus and white balance
    //Cap_setAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE, 0);
    //Cap_setAutoProperty(ctx, streamID, CAPPROPID_FOCUS, 0);
    //Cap_setAutoProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, 0);
    //Cap_setAutoProperty(ctx, streamID, CAPPROPID_GAIN, 0);

    std::vector<uint8_t> m_buffer;
    m_buffer.resize(finfo.width*finfo.height*3);
    uint32_t counter = 0;
    while(counter < 30)
    {
        if (Cap_hasNewFrame(ctx, streamID) == 1)
        {
            Cap_captureFrame(ctx, streamID, &m_buffer[0], m_buffer.size());
            writeBufferAsPPM(counter, finfo.width, finfo.height, &m_buffer[0], m_buffer.size());
            counter++;
            printf("Captured frames: %d\r", counter);
            fflush(stdout);
        }
    };    

    printf("\n\n");
    Cap_closeStream(ctx, streamID);

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


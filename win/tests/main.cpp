/*

    openpnp test application

    Niels Moseley

*/
#include <stdio.h>
#include <conio.h>

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
    
    getch();

    Cap_closeStream(ctx, streamID);

    CapResult result = Cap_releaseContext(ctx);

    return 0;
}


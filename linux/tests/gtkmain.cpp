/*

    openpnp test application

    Niels Moseley

*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <chrono> 

#include "openpnp-capture.h"
#include "../common/context.h"
#include "../common/logging.h"


typedef struct 
{
    GtkImage    *image;
    int32_t     rows, cols, stride;
    CapContext  ctx;
    int32_t     streamID;
    bool        takeSnapshot;
    uint32_t    snapshotCounter;

    int32_t     zoom,gain,exposure,wbalance,focus;
    int32_t     exposure_step;
    int32_t     gstep,wbstep;
    int32_t     fps;
} CallbackInfo;
 

void free_pixels(guchar *pixels, gpointer data) 
{
    free(pixels);
}


static void activate(GtkApplication* app,
    gpointer user_data)
{
    GtkWidget *window;

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "OpenPnP Capture");
    gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);
    gtk_widget_show_all (window);
}

static void triggerSnapshot(GtkWidget *widget,
    gpointer data)
{
    CallbackInfo *id = (CallbackInfo*)data;
    id->takeSnapshot = true;
    printf("Snapshot triggered!\n");
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
    printf("--= Automatic =--\n");
    printf("White balance: ");
    showAutoProperty(ctx, streamID, CAPPROPID_WHITEBALANCE);

    printf("Exposure     : ");
    showAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE);

    printf("Focus        : ");
    showAutoProperty(ctx, streamID, CAPPROPID_FOCUS);

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

int updatePicture(gpointer data) 
{
    //static int N = 0;
    //if (N > 100) return FALSE; // stop timer
 
    CallbackInfo *id = (CallbackInfo*)data;
    GdkPixbuf *pb = gtk_image_get_pixbuf(id->image);
    gdk_pixbuf_fill(pb, 0); // clear to black
    guchar *g = gdk_pixbuf_get_pixels(pb);

    if (Cap_hasNewFrame(id->ctx, id->streamID)==1)
    {
        if (Cap_captureFrame(id->ctx, id->streamID,
            g, id->cols * id->rows * 3) == CAPRESULT_OK)
        {
            LOG(LOG_VERBOSE, "Cap ACK\n");
            gtk_image_set_from_pixbuf(GTK_IMAGE(id->image), pb);    
            if (id->takeSnapshot)
            {
                id->takeSnapshot = false;
                writeBufferAsPPM(id->snapshotCounter++, id->cols, id->rows, g, id->cols * id->rows * 3);
            }
        }
        else
        {
            LOG(LOG_VERBOSE, "Cap NACK\n");
        }
    }
    return TRUE; // continue timer
}

void estimateFrameRate(CapContext ctx, int32_t streamID)
{
    std::chrono::time_point<std::chrono::system_clock> tstart, tend;
    tstart = std::chrono::system_clock::now();
    uint32_t fstart = Cap_getStreamFrameCount(ctx, streamID);
    usleep(2000000);    // 2-second wait
    uint32_t fend = Cap_getStreamFrameCount(ctx, streamID);
    tend = std::chrono::system_clock::now();
    std::chrono::duration<double> fsec = tend-tstart;
    uint32_t frames = fend - fstart;
    printf("Frames = %d\n", frames);
    std::chrono::milliseconds d = std::chrono::duration_cast<std::chrono::milliseconds>(fsec);
    printf("Measured fps=%5.2f\n", 1000.0f*frames/static_cast<float>(d.count()));
    fflush(stdout);
}



gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data) 
{
    if (data == nullptr)
    {
        return FALSE;
    }

    if (event == nullptr)
    {
        return FALSE;
    }

    CallbackInfo *ptr = (CallbackInfo*)data;
    CapContext ctx = ptr->ctx;
    int32_t streamID = ptr->streamID;

    switch(event->keyval)
    {
    case 'q':
        gtk_main_quit();
        return TRUE;
    case '+':
        ptr->exposure += ptr->exposure_step;
        Cap_setProperty(ctx, streamID, CAPPROPID_EXPOSURE, ptr->exposure);
        printf("exposure: %d\r", ptr->exposure);
        fflush(stdout);        
        return TRUE;
    case '-':
        ptr->exposure -= ptr->exposure_step;
        Cap_setProperty(ctx, streamID, CAPPROPID_EXPOSURE, ptr->exposure);
        printf("exposure: %d\r", ptr->exposure);
        fflush(stdout);        
        return TRUE;
    case '0':
        printf("0");
        fflush(stdout);
        ptr->exposure = 0;
        Cap_setProperty(ctx, streamID, CAPPROPID_EXPOSURE, ptr->exposure);
        return TRUE;
    case '1':
        printf("manual exposure\n");
        fflush(stdout);
        Cap_setAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE, 0);
        return TRUE;
    case '2':
        printf("auto exposure\n");
        fflush(stdout);
        Cap_setAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE, 1);
        return TRUE;
#if 0        
    case '3':
        ptr->fps--;
        Cap_setFrameRate(ctx, streamID, ptr->fps);
        printf("framerate = %d\n", ptr->fps);
        fflush(stdout);
        return TRUE;
    case '4':
        ptr->fps++;
        Cap_setFrameRate(ctx, streamID, ptr->fps);
        printf("framerate = %d\n", ptr->fps);
        fflush(stdout);
        return TRUE;
#endif        
    case 'f':
        Cap_setProperty(ctx, streamID, CAPPROPID_FOCUS, ++ptr->focus);
        printf("focus = %d     \r", ptr->focus);
        fflush(stdout);
        return TRUE;
    case 'g':
        Cap_setProperty(ctx, streamID, CAPPROPID_FOCUS, --ptr->focus);
        printf("focus = %d     \r", ptr->focus);
        fflush(stdout);
        return TRUE;
    case 'z':
        Cap_setProperty(ctx, streamID, CAPPROPID_ZOOM, ++ptr->zoom);
        printf("zoom = %d     \r", ptr->zoom);
        fflush(stdout);
        return TRUE;
    case 'x':
        Cap_setProperty(ctx, streamID, CAPPROPID_ZOOM, --ptr->zoom);
        printf("zoom = %d     \r", ptr->zoom);
        fflush(stdout);
        return TRUE;
    case '[':
        ptr->wbalance -= ptr->wbstep;
        Cap_setProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, ptr->wbalance);
        printf("wbal = %d     \r", ptr->wbalance);
        fflush(stdout);
        return TRUE;
    case ']':
        ptr->wbalance += ptr->wbstep; 
        Cap_setProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, ptr->wbalance);
        printf("wbal = %d     \r", ptr->wbalance);
        fflush(stdout);
        return TRUE;
    case 'a':
        ptr->gain -= ptr->gstep;
        Cap_setProperty(ctx, streamID, CAPPROPID_GAIN, ptr->gain);
        printf("gain = %d     \r", ptr->gain);
        fflush(stdout);
        return TRUE;
    case 's':
        ptr->gain += ptr->gstep;
        Cap_setProperty(ctx, streamID, CAPPROPID_GAIN, ptr->gain);
        printf("gain = %d     \r", ptr->gain);
        fflush(stdout);
        return TRUE;
    case 'd':
        printf("Camera configuration:\n");
        showProperties(ctx, streamID);
        showAutoProperties(ctx, streamID);
        printf("\n");
        fflush(stdout);
        return TRUE;
    case 'p':
        printf("Estimating frame rate..\n");
        estimateFrameRate(ctx, streamID);
        fflush(stdout);
        return TRUE;
    case 'w':
        printf("Writing frame to disk..\n");
        ptr->takeSnapshot = true;
        return TRUE;
 
    default: 
        return FALSE;
    }
    return FALSE;
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


int main (int argc, char *argv[])
{
    // https://cboard.cprogramming.com/c-programming/172801-how-display-2d-array-image-using-gtk3plus.html

    GtkApplication *app;
    GtkWidget *button;
    GtkWidget *button_box;

    int status;

    uint32_t deviceFormatID = 0;
    uint32_t deviceID       = 0;

    gtk_init(&argc, &argv);

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

            printf("  Format ID %d: %d x %d pixels  %d fps(max)  FOURCC=%s\n",
                j, finfo.width, finfo.height, finfo.fps, fourccString.c_str());
        }
    }

    // abort if the format was -1
    if (deviceFormatID == 0xFFFFFFFF)
    {
        LOG(LOG_INFO, "Done.\n");
        return 0;
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
    int32_t exmax, exmin, edefault;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_EXPOSURE, 
            &exmin, &exmax, &edefault) == CAPRESULT_OK)
    {
        exposure = (exmax + exmin) / 2;
        Cap_setProperty(ctx, streamID, CAPPROPID_EXPOSURE, exposure);
        printf("Set exposure to %d\n", exposure);
        printf("Default exposure = %d\n",edefault);
    }
    else
    {
        printf("Could not get exposure limits.\n");
    }

    // set focus in the middle of the range
    int32_t focus = 0;
    int32_t fomax, fomin, fodefault;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_FOCUS, 
            &fomin, &fomax, &fodefault) == CAPRESULT_OK)
    {
        focus = (fomax + fomin) / 2;
        Cap_setProperty(ctx, streamID, CAPPROPID_FOCUS, focus);
        printf("Set focus to %d\n", focus);
        printf("Default focus = %d\n",fodefault);
    }
    else
    {
        printf("Could not get focus limits.\n");
    }

    // set zoom in the middle of the range
    int32_t zoom = 0;
    int32_t zomax, zomin, zodefault;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_ZOOM, 
            &zomin, &zomax, &zodefault) == CAPRESULT_OK)
    {
        zoom = zomin;
        Cap_setProperty(ctx, streamID, CAPPROPID_ZOOM, zoom);
        printf("Set zoom to %d\n", zoom);
        printf("Default focus = %d\n",zodefault);
    }
    else
    {
        printf("Could not get zoom limits.\n");
    }

    // set white balance in the middle of the range
    int32_t wbalance = 0;
    int32_t wbmax, wbmin, wbdefault;
    int32_t wbstep = 0;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_WHITEBALANCE, 
            &wbmin, &wbmax, &wbdefault) == CAPRESULT_OK)
    {
        wbalance = (wbmax+wbmin)/2;
        wbstep = (wbmax-wbmin) / 20;
        Cap_setProperty(ctx, streamID, CAPPROPID_WHITEBALANCE, wbalance);
        printf("Set white balance to %d\n", wbalance);
        printf("Default white balance = %d\n",wbdefault);
    }
    else
    {
        printf("Could not get white balance limits.\n");
    }

    // set gain in the middle of the range
    int32_t gain = 0;
    int32_t gmax, gmin, gdefault;
    int32_t gstep = 0;
    if (Cap_getPropertyLimits(ctx, streamID, CAPPROPID_GAIN, 
            &gmin, &gmax, &gdefault) == CAPRESULT_OK)
    {
        gstep = (gmax-gmin) / 20;
        Cap_setProperty(ctx, streamID, CAPPROPID_GAIN, gain);
        printf("Set gain to %d (min=%d max=%d)\n", gain, gmin, gmax);
        printf("Default gain = %d\n",gdefault);
    }
    else
    {
        printf("Could not get gain limits.\n");
    }


    // create GTK image
    CallbackInfo id;
    id.rows = finfo.height;
    id.cols = finfo.width;
    id.stride = finfo.width * 3;
    id.stride += (4 - id.stride % 4) % 4; // ensure multiple of 4
    id.ctx  = ctx;
    id.streamID = streamID;
    id.takeSnapshot = false;
    id.snapshotCounter = 0;
    
    id.exposure = exposure;
    id.exposure_step = (exmin+exmax)/20;
    id.gain = gain;
    id.wbalance = wbalance;
    id.focus = focus;
    id.gstep = gstep;
    id.wbstep = wbstep;
    id.zoom = zoom;
    id.fps  = finfo.fps;

    guchar *pixels = (guchar *)calloc(finfo.height * id.stride, 1);

    GdkPixbuf *pb = gdk_pixbuf_new_from_data
    (
        pixels,
        GDK_COLORSPACE_RGB,     // colorspace
        0,                      // has_alpha
        8,                      // bits-per-sample
        finfo.width,            // cols
        finfo.height,           // cols
        id.stride,              // rowstride
        free_pixels,            // destroy_fn
        NULL                    // destroy_fn_data
    );

    id.image = GTK_IMAGE(gtk_image_new_from_pixbuf(pb));

#if 0
    app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
#endif

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Image");
    gtk_window_set_default_size(GTK_WINDOW(window), finfo.width, finfo.height);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    

    button_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER (window), button_box);

    button = gtk_button_new_with_label ("Save Frame to .ppm");
    g_signal_connect (button, "clicked", G_CALLBACK (triggerSnapshot), &id);
    gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(id.image));
    gtk_container_add(GTK_CONTAINER(button_box), button);

    g_timeout_add(50,         // milliseconds
                updatePicture,  // handler function
                &id);        // data

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(window, "key-press-event", G_CALLBACK (on_key_press), &id);

    printf("=== KEY MAPPINGS ===\n");
    printf("Press q to exit.\n");
    printf("Press + or - to change the exposure.\n");
    printf("Press f or g to change the focus.\n");
    printf("Press z or x to change the zoom.\n");
    printf("Press a or s to change the gain.\n");
    printf("Press [ or ] to change the white balance.\n");
    printf("Press d to show camera configuration.\n");
    printf("Press p to estimate the actual frame rate.\n");
    printf("Press w to write the current frame to a PPM file.\n");
    fflush(stdout);

    printf("Camera configuration:\n");
    showProperties(ctx, streamID);
    showAutoProperties(ctx, streamID);
    printf("\n");
    fflush(stdout);

    gtk_widget_show_all(window);
    gtk_main();

    Cap_closeStream(ctx, streamID);
    CapResult result = Cap_releaseContext(ctx);

  return status;
}

#if 0






int main(int argc, char*argv[])
{    


    std::vector<uint8_t> m_buffer;
    m_buffer.resize(finfo.width*finfo.height*3);

#if 0    
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
    uint32_t frameWriteCounter=0;    
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

    Cap_closeStream(ctx, streamID);

    CapResult result = Cap_releaseContext(ctx);

    return 0;
}

#endif
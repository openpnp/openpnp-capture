/*

    openpnp test application

    Niels Moseley

*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <gtk/gtk.h>

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
} TimerCallbackInfo;
 

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
    TimerCallbackInfo *id = (TimerCallbackInfo*)data;
    id->takeSnapshot = true;
    printf("Snapshot triggered!\n");
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
 
    TimerCallbackInfo *id = (TimerCallbackInfo*)data;
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

    Cap_setAutoProperty(ctx, streamID, CAPPROPID_EXPOSURE, 0);

    // create GTK image
    TimerCallbackInfo id;
    id.rows = finfo.height;
    id.cols = finfo.width;
    id.stride = finfo.width * 3;
    id.stride += (4 - id.stride % 4) % 4; // ensure multiple of 4
    id.ctx  = ctx;
    id.streamID = streamID;
    id.takeSnapshot = false;
    id.snapshotCounter = 0;
    
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

    g_timeout_add(250,         // milliseconds
                updatePicture,  // handler function
                &id);        // data

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
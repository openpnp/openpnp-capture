/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Stream class

*/

#ifndef stream_h
#define stream_h

#include <windows.h>
#include <dshow.h>
#include <Vidcap.h>
#include <Ksmedia.h>

#include <stdint.h>
#include <vector>
#include <mutex>
#include "logging.h"
#include "samplegrabber.h"

class Context;      // pre-declaration
struct deviceInfo;  // pre-declaration
class Stream;       // pre-declaration

/** A class to handle callbacks from the video subsystem,
    A call is made for every frame.

    Note that the callback will be called by the DirectShow thread
    and should return as quickly as possible to avoid interference
    with the capturing process.
*/
class StreamCallbackHandler : public ISampleGrabberCB
{
public:
    StreamCallbackHandler(Stream *stream) : m_stream(stream)
    {
        m_callbackCounter = 0;
    }

    ~StreamCallbackHandler()
    {
        LOG(LOG_INFO, "Callback counter = %d\n", m_callbackCounter);
    }

    /** callback handler used in this library */
    virtual HRESULT __stdcall SampleCB(double time, IMediaSample* sample) override;

    /** alternate callback handler (not used) */
    virtual HRESULT __stdcall BufferCB(double time, BYTE* buffer, long len) override
    {
        //m_callbackCounter++;
        return S_OK;
    }

    /** function implementation required by ISampleGrabberCB base class */
    virtual HRESULT __stdcall QueryInterface( REFIID iid, LPVOID *ppv )
    {
        if( iid == IID_ISampleGrabberCB || iid == IID_IUnknown )
        {
            *ppv = (void *) static_cast<ISampleGrabberCB*>( this );
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    /** function implementation required by ISampleGrabberCB base class */
    virtual ULONG	__stdcall AddRef()
    {
        return 1;
    }

    /** function implementation required by ISampleGrabberCB base class */
    virtual ULONG	__stdcall Release()
    {
        return 2;
    }

    uint32_t getCallbackCounter() const
    {
        return m_callbackCounter;
    }

    void reset()
    {
        m_callbackCounter = 0;
    }


private:
    std::mutex  m_bufferMutex;      ///< mutex to protect frame buffer access
    Stream*     m_stream;
    uint32_t    m_callbackCounter;
};


/** The stream class handles the capturing of a single device */
class Stream
{
friend StreamCallbackHandler;

public:
    Stream();
    ~Stream();

    /** Open a capture stream to a device and request a specific (internal) stream format. 
        When succesfully opened, capturing starts immediately.
    */
    bool open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, uint32_t fourCC);

    /** Close a capture stream */
    void close();

    /** Returns true if a new frame is available for reading using 'captureFrame'. 
        The internal new frame flag is reset by captureFrame.
    */
    bool hasNewFrame();

    /** Retrieve the most recently captured frame and copy it in a
        buffer pointed to by RGBbufferPtr. The maximum buffer size 
        must be supplied in RGBbufferBytes.
    */
    bool captureFrame(uint8_t *RGBbufferPtr, uint32_t RGBbufferBytes);
    
    /** Returns true if the stream is open and capturing */
    bool isOpen() const
    {
        return m_isOpen;
    }

    /** Return the FOURCC media type of the stream */
    uint32_t getFOURCC();

    /** Return the number of frames captured.
        FIXME: protect by mutex 
    */
    uint32_t getFrameCount() const
    {
        return m_frames;
    }

protected:
    void submitBuffer(uint8_t* ptr, size_t bytes);
    void dumpCameraProperties();

    Context*    m_owner;                    ///< The context object associated with this stream

    uint32_t    m_width;                    ///< The width of the frame in pixels
    uint32_t    m_height;                   ///< The height of the frame in pixels
    bool        m_isOpen;

    IFilterGraph2*  m_graph;
    IMediaControl*  m_control;
    IBaseFilter*    m_sourceFilter;
    IBaseFilter*    m_sampleGrabberFilter;
    ISampleGrabber* m_sampleGrabber;
    ICaptureGraphBuilder2* m_capture;
    IAMCameraControl* m_camControl;

    /** Each time a new frame is available, the DirectShow subsystem
        will call the callback handler */
    StreamCallbackHandler *m_callbackHandler;

    std::mutex  m_bufferMutex;              ///< mutex to protect m_frameBuffer and m_newFrame
    bool        m_newFrame;                 ///< new frame buffer flag
    std::vector<uint8_t> m_frameBuffer;     ///< raw frame buffer
    uint32_t    m_frames;                   ///< number of frames captured

    VIDEOINFOHEADER m_videoInfo;            ///< video information of current captured stream
};

#endif
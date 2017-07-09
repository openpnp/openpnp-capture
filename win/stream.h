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
#include <stdint.h>
#include "logging.h"
#include "samplegrabber.h"

class Context;      // pre-declaration
struct deviceInfo;  // device info class
class Stream;

/** A class to handle callbacks from the video subsystem,
    A call is made for every frame */

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

    virtual HRESULT __stdcall SampleCB(double time, IMediaSample* sample) override
    {
        m_callbackCounter++;
        return S_OK;
    }

    virtual HRESULT __stdcall BufferCB(double time, BYTE* buffer, long len) override
    {
        m_callbackCounter++;
        return S_OK;
    }

    virtual HRESULT __stdcall QueryInterface( REFIID iid, LPVOID *ppv )
    {
        if( iid == IID_ISampleGrabberCB || iid == IID_IUnknown )
        {
            *ppv = (void *) static_cast<ISampleGrabberCB*>( this );
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    virtual ULONG	__stdcall AddRef()
    {
        return 1;
    }

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
    Stream*  m_stream;
    uint32_t m_callbackCounter;
};


/** The stream class handles the capturing of a single device */
class Stream
{
public:
    Stream();
    ~Stream();

    bool open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, uint32_t fourCC);
    void close();

    bool hasNewFrame();
    void captureFrame(uint8_t *RGBbufferPtr, uint32_t RGBbufferBytes);

protected:
    Context*                m_owner;

    uint32_t    m_width;
    uint32_t    m_height;

    IFilterGraph2*  m_graph;
    IMediaControl*  m_control;
    IBaseFilter*    m_sourceFilter;
    IBaseFilter*    m_sampleGrabberFilter;
    ISampleGrabber* m_sampleGrabber;
    ICaptureGraphBuilder2* m_capture;

    StreamCallbackHandler *m_callbackHandler;
};

#endif
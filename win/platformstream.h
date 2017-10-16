/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Stream class

*/

#ifndef win32platform_stream_h
#define win32platform_stream_h

#include <windows.h>
#include <dshow.h>
#include <Vidcap.h>
#include <Ksmedia.h>

#include <stdint.h>
#include <vector>
#include <mutex>
#include "../common/logging.h"
#include "../common/stream.h"
#include "samplegrabber.h"


class Context;         // pre-declaration
class PlatformStream;  // pre-declaration

/** A class to handle callbacks from the video subsystem,
    A call is made for every frame.

    Note that the callback will be called by the DirectShow thread
    and should return as quickly as possible to avoid interference
    with the capturing process.
*/
class StreamCallbackHandler : public ISampleGrabberCB
{
public:
    StreamCallbackHandler(PlatformStream *stream) : m_stream(stream)
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
    PlatformStream* m_stream;
    uint32_t        m_callbackCounter;
};


/** The stream class handles the capturing of a single device */
class PlatformStream : public Stream
{
friend StreamCallbackHandler;

public:
    PlatformStream();
    virtual ~PlatformStream();

    /** Open a capture stream to a device and request a specific (internal) stream format. 
        When succesfully opened, capturing starts immediately.
    */
    virtual bool open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, 
        uint32_t fourCC, uint32_t fps) override;

    /** Close a capture stream */
    virtual void close() override;

    /** Returns true if a new frame is available for reading using 'captureFrame'. 
        The internal new frame flag is reset by captureFrame.
    */
    bool hasNewFrame();

    /** Retrieve the most recently captured frame and copy it in a
        buffer pointed to by RGBbufferPtr. The maximum buffer size 
        must be supplied in RGBbufferBytes.
    */
    bool captureFrame(uint8_t *RGBbufferPtr, uint32_t RGBbufferBytes);

    /** set the frame rate */
    virtual bool setFrameRate(uint32_t fps) override;

    /** Return the FOURCC media type of the stream */
    virtual uint32_t getFOURCC() override;

    /** get the limits of a camera/stream property (exposure, zoom etc) */
    virtual bool getPropertyLimits(uint32_t propID, int32_t *min, int32_t *max, int32_t *dValue) override;

    /** set property (exposure, zoom etc) of camera/stream */
    virtual bool setProperty(uint32_t propID, int32_t value) override;

    /** set automatic state of property (exposure, zoom etc) of camera/stream */
    virtual bool setAutoProperty(uint32_t propID, bool enabled) override;

    /** get property (exposure, zoom etc) of camera/stream */
    virtual bool getProperty(uint32_t propID, int32_t &outValue) override;
    
    /** get automatic state of property (exposure, zoom etc) of camera/stream */
    virtual bool getAutoProperty(uint32_t propID, bool &enabled) override;

protected:
    /** A re-implementation of Stream::submitBuffer with BGR to RGB conversion */
    virtual void submitBuffer(const uint8_t *ptr, size_t bytes) override;

    /** Add the Direct show filter graph to the object list so
        GraphEdt.exe can see it - for debugging purposes only.
        See: https://msdn.microsoft.com/en-us/library/windows/desktop/dd390650(v=vs.85).aspx    
    */
    HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);

    /** Remove the Direct show filter graph from the object list 
         - for debugging purposes only. */
    void RemoveFromRot(DWORD pdwRegister);

    /** Write filter graph to 'filtergraph.grf' file - for debugging purposes only.*/
    HRESULT SaveGraphFile(IGraphBuilder *pGraph);

    /** get DirectShow property + flags helper function */
    bool getDSProperty(uint32_t propID, long &value, long &flags);

    void dumpCameraProperties();

    IFilterGraph2*  m_graph;
    IMediaControl*  m_control;
    IBaseFilter*    m_sourceFilter;
    IBaseFilter*    m_sampleGrabberFilter;
    IBaseFilter*    m_nullRenderer;
    ISampleGrabber* m_sampleGrabber;
    ICaptureGraphBuilder2* m_capture;
    IAMCameraControl* m_camControl;
    IAMVideoProcAmp* m_videoProcAmp;

    /** Each time a new frame is available, the DirectShow subsystem
        will call the callback handler */
    StreamCallbackHandler *m_callbackHandler;

    VIDEOINFOHEADER m_videoInfo;            ///< video information of current captured stream

    DWORD dwRotRegister;    ///< for exposing the filtergraph to GraphEdt
};

#endif
/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Stream class

*/

#include "stream.h"
#include "context.h"
#include "scopedcomptr.h"

Stream::Stream() :
    m_owner(nullptr),
    m_graph(nullptr),
    m_control(nullptr),
    m_callbackHandler(nullptr)
{

}

Stream::~Stream()
{
    close();
}

void Stream::close()
{
    LOG(LOG_INFO, "closing stream\n");

    if (m_sampleGrabberFilter != nullptr)
    {
        m_sampleGrabberFilter->Stop();
    }

    if (m_sourceFilter != nullptr)
    {
	    m_sourceFilter->Stop();
    }

    SafeRelease(&m_graph);
    SafeRelease(&m_control);
    SafeRelease(&m_capture);
    SafeRelease(&m_sourceFilter);
    SafeRelease(&m_sampleGrabberFilter);
    SafeRelease(&m_sampleGrabber);

    if (m_callbackHandler != 0)
    {
        delete m_callbackHandler;
    }
}


bool Stream::open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, uint32_t fourCC)
{
    if (m_owner != nullptr)
    {
        // stream wasn't closed!
        LOG(LOG_ERR,"open() was called on an active stream!\n");        
        return false;
    }

    if (owner == nullptr)
    {
        LOG(LOG_ERR,"open() was with owner=NULL!\n");        
        return false;
    }

    if (owner == nullptr)
    {
        LOG(LOG_ERR,"open() was with device=NULL!\n");
        return false;
    }

    m_owner = owner;

    // Create the filter graph object.
    HRESULT hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IFilterGraph2, (void **) &m_graph);
    if (FAILED(hr))
    {
        LOG(LOG_ERR,"Could not create IFilterGraph2\n");
        return false;
    }

	//create the CaptureGraphBuilder
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2,NULL,CLSCTX_INPROC_SERVER,IID_ICaptureGraphBuilder2,(void**) &m_capture);
	if (FAILED(hr))
    {
        LOG(LOG_ERR,"Could not create ICaptureGraphBuilder2\n");
        return false;        
    }

    m_capture->SetFiltergraph(m_graph);

    //get the controller for the graph
	hr = m_graph->QueryInterface(IID_IMediaControl, (void**) &m_control);    
    if (FAILED(hr))
    {
        LOG(LOG_ERR,"Could not create IMediaControl\n");
        return false;
    }

    hr = m_graph->AddSourceFilterForMoniker(device->m_moniker, 0, device->m_filterName.c_str(), &m_sourceFilter);
    if (hr != S_OK)
    {
        LOG(LOG_ERR,"Could add source filter to filter graph (HRESULT=%08X)\n", hr);
        return false;        
    } 

    //create a samplegrabber filter for the device
    hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,IID_IBaseFilter, (void**)&m_sampleGrabberFilter);
    if (hr < 0)
    {
        LOG(LOG_ERR,"Could create sample grabber filter\n");
        return false;        
    }

    //set mediatype on the samplegrabber
    hr = m_sampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&m_sampleGrabber);
    if (hr != S_OK)
    {
        LOG(LOG_ERR,"Could not create ISampleGrabber (HRESULT=%08X)\n", hr);
        return false;
    }

    // generate a unique name for each sample grabber filter
    // used in the system.
    std::wstring filtername(L"SGF_");
    filtername.append(device->m_filterName);
    hr = m_graph->AddFilter(m_sampleGrabberFilter, filtername.c_str());
    if (hr < 0)
    {
        LOG(LOG_ERR,"Could not add ISampleGrabber filter to graph (HRESULT=%08X)\n", hr);
        return false;
    }

    //set the media type
    AM_MEDIA_TYPE mt;
    memset(&mt, 0, sizeof(AM_MEDIA_TYPE));

    mt.majortype	= MEDIATYPE_Video;
    mt.subtype		= MEDIASUBTYPE_RGB24; 

    hr = m_sampleGrabber->SetMediaType(&mt);
    if (hr != S_OK)
    {
        LOG(LOG_ERR,"Could not set the samplegrabber media type to 24-bit RGB\n");
        return false;
    }
    
    //add the callback to the samplegrabber
    if (m_callbackHandler == nullptr)
    {
        m_callbackHandler = new StreamCallbackHandler(this);
    }
    else
    {
        m_callbackHandler->reset();
    }

    hr = m_sampleGrabber->SetCallback(m_callbackHandler,0);
    if (hr != S_OK)
    {
        LOG(LOG_ERR,"Could not set callback on sample grabber (HRESULT=%08X)\n", hr);
        return false;
    }       

    hr = m_capture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, m_sourceFilter, m_sampleGrabberFilter, NULL);
    if (hr < 0)
    {
        LOG(LOG_ERR,"Error calling RenderStream (HRESULT=%08X)\n", hr);
        return false;
    }

    LONGLONG start=0, stop=MAXLONGLONG;
    hr = m_capture->ControlStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_sourceFilter, &start, &stop, 1,2);
    if (hr < 0)
    {
        LOG(LOG_ERR,"Could not start the video stream (HRESULT=%08X)\n", hr);
        return false;
    }    

    // look up the media type:
    m_width = 0;
    m_height = 0;
    AM_MEDIA_TYPE* info = new AM_MEDIA_TYPE();
    hr = m_sampleGrabber->GetConnectedMediaType(info);
    if ( hr == S_OK )
    {
        if (info->formattype == FORMAT_VideoInfo)
        {
            const VIDEOINFOHEADER * vi = reinterpret_cast<VIDEOINFOHEADER*>( info->pbFormat );
            m_width  = vi->bmiHeader.biWidth;
            m_height = vi->bmiHeader.biHeight;
            LOG(LOG_INFO, "Width = %d, Height = %d\n", m_width, m_height);            
        }
        CoTaskMemFree( info->pbFormat );        
    }
    free(info);	

    LOG(LOG_INFO,"Stream to device %s opened\n", device->m_name.c_str());

    //FIXME: add error handling
    m_control->Run();

	hr = m_sampleGrabberFilter->Run(0);

	hr = m_sourceFilter->Run(0);

    return true;
}

bool Stream::hasNewFrame()
{
    return true;
}

void Stream::captureFrame(uint8_t *RGBbufferPtr, uint32_t RGBbufferBytes)
{
}

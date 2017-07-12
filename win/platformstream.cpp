/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Stream class

*/

#include "platformdeviceinfo.h"
#include "platformstream.h"
#include "platformcontext.h"
#include "scopedcomptr.h"

Stream* createPlatformStream()
{
    return new PlatformStream();
}

// **********************************************************************
//   StreamCallbackHandler
// **********************************************************************

HRESULT __stdcall StreamCallbackHandler::SampleCB(double time, IMediaSample* sample)
{
    if (sample == nullptr)
    {
        return S_OK;
    }

    m_callbackCounter++;

    size_t bytes = sample->GetActualDataLength();
    uint8_t *ptr;
    if ((sample->GetPointer(&ptr) == S_OK) && (m_stream != nullptr))
    {
        m_stream->submitBuffer(ptr, bytes);
    }
    return S_OK;
}



// **********************************************************************
//   PlatformStream
// **********************************************************************

PlatformStream::PlatformStream() : 
    Stream(),
    m_graph(nullptr),
    m_control(nullptr),
    m_callbackHandler(nullptr),
    m_sampleGrabberFilter(nullptr),
    m_sourceFilter(nullptr),
    m_sampleGrabber(nullptr),
    m_camControl(nullptr)
{

}

PlatformStream::~PlatformStream()
{
    close();
}

void PlatformStream::close()
{
    LOG(LOG_INFO, "closing stream\n");

    if (m_control != nullptr)
    {
        m_control->Stop();
    }

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
    SafeRelease(&m_camControl);

    if (m_callbackHandler != 0)
    {
        delete m_callbackHandler;
    }

    m_owner = nullptr;
    m_width = 0;
    m_height = 0;
    m_frameBuffer.resize(0);
    m_isOpen = false;    
}


bool PlatformStream::open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, uint32_t fourCC)
{
    if (m_isOpen)
    {
        LOG(LOG_INFO,"open() was called on an active stream.\n");
        close();
    }

    if (owner == nullptr)
    {
        LOG(LOG_ERR,"open() was with owner=NULL!\n");        
        return false;
    }

    if (device == nullptr)
    {
        LOG(LOG_ERR,"open() was with device=NULL!\n");
        return false;
    }

    platformDeviceInfo *dinfo = dynamic_cast<platformDeviceInfo*>(device);
    if (dinfo == NULL)
    {
        LOG(LOG_CRIT, "Could not cast deviceInfo* to platfromDeviceInfo*!");
        return false;
    }

    m_owner = owner;
    m_frames = 0;
    m_width = 0;
    m_height = 0;    

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

    hr = m_graph->AddSourceFilterForMoniker(dinfo->m_moniker, 0, dinfo->m_filterName.c_str(), &m_sourceFilter);
    if (hr != S_OK)
    {
        LOG(LOG_ERR,"Could add source filter to filter graph (HRESULT=%08X)\n", hr);
        return false;        
    } 

    // create camera control interface for exposure control etc . 
    hr = m_sourceFilter->QueryInterface(IID_IAMCameraControl, (void **)&m_camControl); 
    if (hr != S_OK) 
    {
        LOG(LOG_ERR,"Could not create IAMCameraControl\n");
        return false;  
    }

    // FIXME/TODO: we should still be able to work with the camera when the
    //       IAMCameraControl was not implemented.    
    // disable auto exposure
    m_camControl->Set(CameraControl_Exposure, -7, CameraControl_Flags_Manual | KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE);
    dumpCameraProperties();

    //create a samplegrabber filter for the device
    hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,IID_IBaseFilter, (void**)&m_sampleGrabberFilter);
    if (hr < 0)
    {
        LOG(LOG_ERR,"Could not create sample grabber filter\n");
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
    filtername.append(dinfo->m_filterName);
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
    AM_MEDIA_TYPE* info = new AM_MEDIA_TYPE();
    hr = m_sampleGrabber->GetConnectedMediaType(info);
    if ( hr == S_OK )
    {
        if (info->formattype == FORMAT_VideoInfo)
        {
            const VIDEOINFOHEADER * vi = reinterpret_cast<VIDEOINFOHEADER*>( info->pbFormat );
            m_width  = vi->bmiHeader.biWidth;
            m_height = vi->bmiHeader.biHeight;
            memcpy(&m_videoInfo, vi, sizeof(VIDEOINFOHEADER)); // save video header information
            LOG(LOG_INFO, "Width = %d, Height = %d\n", m_width, m_height);

            //FIXME: for now, just set the frame buffer size to
            //       width*height*3 for 24 RGB raw images
            m_frameBuffer.resize(m_width*m_height*3);                  
        }
        CoTaskMemFree( info->pbFormat );        
    }
    free(info);	

    LOG(LOG_INFO,"Stream to device %s opened\n", device->m_name.c_str());

    //FIXME: add error handling
    m_control->Run();

	hr = m_sampleGrabberFilter->Run(0);

	hr = m_sourceFilter->Run(0);

    m_isOpen = true;

    return true;
}

uint32_t PlatformStream::getFOURCC()
{
    if (!m_isOpen) return 0;

    if ((m_videoInfo.bmiHeader.biCompression == BI_RGB) || 
        (m_videoInfo.bmiHeader.biCompression == BI_BITFIELDS))
    {
        //FIXME: is this the correct 4CC?
        return MAKEFOURCC('R','G','B',' ');
    }
    else
    {
        return m_videoInfo.bmiHeader.biCompression;
    }
}

void PlatformStream::dumpCameraProperties()
{
    if (m_camControl != 0)
    {
		//query exposure
		long flags, mmin, mmax, delta, defaultValue;
        if (m_camControl->GetRange(CameraControl_Exposure, &mmin, &mmax,
            &delta, &defaultValue, &flags) == S_OK)
        {
            LOG(LOG_INFO, "Exposure min     : %2.3f seconds (%d integer)\n", pow(2.0f, (float)mmin), mmin);
            LOG(LOG_INFO, "Exposure max     : %2.3f seconds (%d integer)\n", pow(2.0f, (float)mmax), mmax);
            LOG(LOG_INFO, "Exposure step    : %d (integer)\n", delta);
            LOG(LOG_INFO, "Exposure default : %2.3f seconds\n", pow(2.0f, (float)defaultValue));		
            LOG(LOG_INFO, "Flags            : %08X\n", flags);
        }
        else
        {
            LOG(LOG_INFO, "Could not get exposure range information\n");
        }

        //query focus
        if (m_camControl->GetRange(CameraControl_Focus, &mmin, &mmax,
            &delta, &defaultValue, &flags) == S_OK)
        {
            LOG(LOG_INFO, "Focus min     : %d integer\n", mmin);
            LOG(LOG_INFO, "Focus max     : %d integer\n", mmax);
            LOG(LOG_INFO, "Focus step    : %d integer\n", delta);
            LOG(LOG_INFO, "Focus default : %d integer\n", defaultValue);
            LOG(LOG_INFO, "Flags         : %08X\n", flags);
        }
        else
        {
            LOG(LOG_INFO, "Could not get focus range information\n");
        }        

        // query zoom
        if (m_camControl->GetRange(CameraControl_Zoom, &mmin, &mmax,
            &delta, &defaultValue, &flags) == S_OK)
        {
            LOG(LOG_INFO, "Zoom min     : %d integer\n", mmin);
            LOG(LOG_INFO, "Zoom max     : %d integer\n", mmax);
            LOG(LOG_INFO, "Zoom step    : %d integer\n", delta);
            LOG(LOG_INFO, "Zoom default : %d integer\n", defaultValue);
            LOG(LOG_INFO, "Flags         : %08X\n", flags);
        }
        else
        {
            LOG(LOG_INFO, "Could not get Zoom range information\n");
        }         

#if 0
		if (m_camControl->get_Exposure(&value, &flags) == S_OK)
		{
			printf("Exposure: %2.3f seconds\n", pow(2.0f, (float)value));
			printf("Flags   : %08X\n", flags);
		}
		else
		{
			printf("Exposure info failed!\n");
		}
#endif       
    }
}

bool PlatformStream::setExposure(int32_t value) 
{
    if (m_camControl != 0)
    {
        long flags, dummy;
        if (m_camControl->Get(CameraControl_Exposure, &dummy, &flags) != S_OK)
        {
            return false;
        }
        if (m_camControl->Set(CameraControl_Exposure, value, flags) != S_OK)
        {
            return false;
        }
        return true;
    }
    return false;
}


bool PlatformStream::setAutoExposure(bool enabled) 
{
    if (m_camControl != 0)
    {
        //FIXME: check return codes.
        if (enabled)
            m_camControl->Set(CameraControl_Exposure, 0, CameraControl_Flags_Auto | KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE);
        else
            m_camControl->Set(CameraControl_Exposure, 0, CameraControl_Flags_Manual | KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE);

        return true;
    }
    return false;
}


bool PlatformStream::getExposureLimits(int32_t *emin, int32_t *emax) 
{
    if ((m_camControl != 0) && (emin != nullptr) && (emax != nullptr))
    {
        //query exposure
		long flags, mmin, mmax, delta, defaultValue;
        if (m_camControl->GetRange(CameraControl_Exposure, &mmin, &mmax,
            &delta, &defaultValue, &flags) == S_OK)
        {   
            *emin = mmin;
            *emax = mmax;
            return true;
        }
    }
    return false;
}

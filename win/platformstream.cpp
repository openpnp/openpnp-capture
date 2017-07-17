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

extern void _FreeMediaType(AM_MEDIA_TYPE& mt);

// Delete a media type structure that was allocated on the heap.
extern void _DeleteMediaType(AM_MEDIA_TYPE *pmt);


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

    //set the desired frame buffer format
    IAMStreamConfig *pConfig = NULL;
    hr = m_capture->FindInterface(&PIN_CATEGORY_CAPTURE, 0, m_sourceFilter, IID_IAMStreamConfig, (void**)&pConfig);
    if (FAILED(hr))
    {
        LOG(LOG_ERR,"Could not create IAMStreamConfig\n");
        return false;
    }

    ScopedComPtr<IAMStreamConfig> streamConfig(pConfig);

    // find the desired video mode
    AM_MEDIA_TYPE *selectedConfig = NULL;

    int iCount = 0, iSize = 0;
    hr = streamConfig->GetNumberOfCapabilities(&iCount, &iSize);
    if (FAILED(hr))
    {
        LOG(LOG_ERR, "Cannot retrieve device capabilities\n");
        return false;
    }

    // Check the size to make sure we pass in the correct structure.
    if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
    {
        bool formatSet = false;
        // Use the video capabilities structure.
        for (int iFormat = 0; iFormat < iCount; iFormat++)
        {
            VIDEO_STREAM_CONFIG_CAPS scc;
            AM_MEDIA_TYPE *pmtConfig;
            hr = streamConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
            if (SUCCEEDED(hr))
            {
                /* Examine the format, and possibly use it. */

                if ((pmtConfig->majortype == MEDIATYPE_Video) &&
                    (pmtConfig->formattype == FORMAT_VideoInfo) &&
                    (pmtConfig->cbFormat >= sizeof (VIDEOINFOHEADER)) &&
                    (pmtConfig->pbFormat != NULL))
                {
                    VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pmtConfig->pbFormat);

                    if ((pVih->bmiHeader.biWidth == width) &&
                        (pVih->bmiHeader.biHeight == height) &&
                        (pVih->bmiHeader.biCompression == fourCC))
                    {
                        streamConfig->SetFormat(pmtConfig);                        
                        formatSet = true;
                        LOG(LOG_INFO, "Capture format set!\n");
                        break;
                    }
                }
                // Delete the media type when you are done.
                _DeleteMediaType(pmtConfig);
            }
        }
        if (!formatSet)
        {
            LOG(LOG_ERR, "Failed to find capture format!\n");
            return false;
        }        
    }
    else
    {
        LOG(LOG_ERR,"Could not find video mode: VIDEO_STREAM_CONFIG_CAPS not found\n");
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

    hr = m_capture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_sourceFilter, m_sampleGrabberFilter, NULL);
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

void PlatformStream::submitBuffer(const uint8_t *ptr, size_t bytes)
{
    m_bufferMutex.lock();
    
    if (m_frameBuffer.size() == 0)
    {
        LOG(LOG_ERR,"Stream::m_frameBuffer size is 0 - cant store frame buffers!\n");
    }

    // Generate warning every 100 frames if the frame buffer is not
    // the expected size. 
    
    const uint32_t wantSize = m_width*m_height*3;
    if ((bytes != wantSize) && ((m_frames % 100) == 0))
    {
        LOG(LOG_WARNING, "Warning: captureFrame received incorrect buffer size (got %d want %d)\n", bytes, wantSize);
    }

    if (bytes <= m_frameBuffer.size())
    {
        // The Win32 API delivers upside-down BGR frames.
        // Conversion to regular RGB frames is done by
        // byte-reversing the buffer
            
        for(size_t y=0; y<m_height; y++)
        {
            uint8_t *dst = &m_frameBuffer[(y*m_width)*3];
            const uint8_t *src = ptr + (m_width*3)*(m_height-y-1);
            for(uint32_t x=0; x<m_width; x++)
            {
                uint8_t b = *src++;
                uint8_t g = *src++;
                uint8_t r = *src++;
                *dst++ = r;
                *dst++ = g;
                *dst++ = b;
            }
        }

        m_newFrame = true; 
        m_frames++;        
    }

    m_bufferMutex.unlock();
}
/*

    OpenPnp-Capture: a video capture subsystem.

    Windows Stream class

    Created by Niels Moseley on 7/6/17.
    Copyright (c) 2017 Niels Moseley.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "platformdeviceinfo.h"
#include "platformstream.h"
#include "platformcontext.h"
#include "scopedcomptr.h"

#include <cmath>

extern HRESULT FindCaptureDevice(IBaseFilter** ppSrcFilter, const wchar_t* wDeviceName);
extern void _FreeMediaType(AM_MEDIA_TYPE& mt);

// Delete a media type structure that was allocated on the heap.
extern void _DeleteMediaType(AM_MEDIA_TYPE *pmt);


Stream* createPlatformStream()
{
    return new PlatformStream();
}

// **********************************************************************
//   Property translation data
// **********************************************************************

struct property_t
{
    uint32_t dsProp;            // Directshow CameraControlProperty or VideoProcAmpProperty
    bool     isCameraControl;   // if true dsProp is CameraControlProperty
};

// the order must be the same as the CAPPROPID indeces!
static const property_t gs_properties[] =
{
    {0, true},                      // dummy
    {CameraControl_Exposure, true}, // exposure
    {CameraControl_Focus, true},
    {CameraControl_Zoom, true},
    {VideoProcAmp_WhiteBalance, false},
    {VideoProcAmp_Gain, false},
    {VideoProcAmp_Brightness, false},
    {VideoProcAmp_Contrast, false},
    {VideoProcAmp_Saturation, false},
    {VideoProcAmp_Gamma, false},
    {VideoProcAmp_Hue, false},
    {VideoProcAmp_Sharpness, false},
    {VideoProcAmp_BacklightCompensation, false},
    {KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY, false}
};


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
    //sample->Release(); //who owns the IMediaSample ?!?
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
    m_nullRenderer(nullptr),
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

    #ifdef _DEBUG
    RemoveFromRot(dwRotRegister);
    #endif

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
    SafeRelease(&m_nullRenderer);
    SafeRelease(&m_videoProcAmp);

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


bool PlatformStream::open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, 
    uint32_t fourCC, uint32_t fps)
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

    hr = FindCaptureDevice(&m_sourceFilter, dinfo->m_devicePath.c_str());
    if (hr != S_OK)
    {
        LOG(LOG_ERR, "Could not find source filter %s\n", dinfo->m_devicePath.c_str());
        return false;
    }

    hr = m_graph->AddFilter(m_sourceFilter, L"Video Capture");
    if (hr != S_OK)
    {
        LOG(LOG_ERR, "Could add source filter to filter graph (HRESULT=%08X)\n", hr);
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
    else
    {
        LOG(LOG_DEBUG,"PlatformStream::open() reveals %d stream capabilities\n", iCount);
    }

    // Check the size to make sure we pass in the correct structure.
    if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
    {
        bool formatSet = false;
        LOG(LOG_VERBOSE, "Searching for correct frame buffer mode..\n");
        LOG(LOG_VERBOSE, "Looking for %d %d  %s..\n", width, height, 
            fourCCToString(fourCC).c_str());
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

                    /* Note: pVih->bmiHeader.biCompression is usually the fourCC
                       except when it is equal to BI_RGB, BI_RLE8, BI_RLE4,
                       BI_BITFIELDS, BI_JPEG or BI_PNG
                    */

                    uint32_t format4CC = pVih->bmiHeader.biCompression;
                    switch(format4CC)
                    {
                    case BI_RGB:
                        format4CC = 'RGB ';
                        break;
                    }

                    LOG(LOG_VERBOSE, "  %d x %d %s\n", pVih->bmiHeader.biWidth, 
                        pVih->bmiHeader.biHeight,
                        fourCCToString(format4CC).c_str());

                    if ((pVih->bmiHeader.biWidth == width) &&
                        (pVih->bmiHeader.biHeight == height) &&
                        (format4CC == fourCC))
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
    m_camControl = nullptr;
    hr = m_sourceFilter->QueryInterface(IID_IAMCameraControl, (void **)&m_camControl); 
    if (hr != S_OK) 
    {
        // note: this is not an error because some cameras do not support camera control
        LOG(LOG_WARNING,"Could not create IAMCameraControl\n");
    }

    dumpCameraProperties();

    // create video processing control interface
    m_videoProcAmp = nullptr;
    hr = m_sourceFilter->QueryInterface(IID_IAMVideoProcAmp, (void **)&m_videoProcAmp); 
    if (hr != S_OK) 
    {
        // note: this is not an error, but in inconvenience
        LOG(LOG_WARNING,"Could not create IAMVideoProcAmp\n");
    }

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

    /* Note: Although I had expected to have to use 'PIN_CATEGORY_CAPTURE',
       using 'PIN_CATEGORY_PREVIEW' gives 30 fps at HD res on a Microsoft LifeCam 3000,
       whereas 'PIN_CATEGORY_CAPTURE' results in 2.5 fps !?!

       In order to not create a default display window, the end-point of the stream
       must be a Null renderer. It can be created by:

        //NULL RENDERER//
        //used to give the video stream somewhere to go to.
        hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)(&m_nullRenderer));
        if (FAILED(hr))
        {
            DebugPrintOut("ERROR: Could not create filter - NullRenderer\n");
            stopDevice(deviceID);
            return hr;
        }
    */

    // only create a valid NULL renderer in release builds!
    // FIXME: is this the behavior we actually want,
    //        or should we use a special define to 
    //        enable the preview window?
    #ifndef _DEBUG
    hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)(&m_nullRenderer));
    if (FAILED(hr))
    {
        LOG(LOG_WARNING,"Could not create a NULL renderer - using NULL ptr instead.");
    }
    else
    {
        // we need to add the filter to the graph to be able to use it.. 
        hr = m_graph->AddFilter(m_nullRenderer, L"NULLRenderer");
        if (hr < 0)
        {
            LOG(LOG_ERR,"Could not add NULL Renderer to graph (HRESULT=%08X)\n", hr);
            return false;
        }
    }
    #endif

    hr = m_capture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, m_sourceFilter, m_sampleGrabberFilter, m_nullRenderer);
    if (hr < 0)
    {
        LOG(LOG_ERR,"Error calling RenderStream (HRESULT=%08X)\n", hr);
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
    dwRotRegister = 0;

    #ifdef _DEBUG    
    //SaveGraphFile(m_graph);
    IGraphBuilder *captureGraph;
    if (SUCCEEDED(m_capture->GetFiltergraph(&captureGraph)))
    {
        hr = AddToRot(captureGraph, &dwRotRegister);
    }
    #endif    

    return true;
}

bool PlatformStream::setFrameRate(uint32_t fps)
{
    //FIXME: implement
    return false;
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
            LOG(LOG_INFO, "Exposure min     : %2.3f seconds (%d integer)\n", std::pow(2.0f, (float)mmin), mmin);
            LOG(LOG_INFO, "Exposure max     : %2.3f seconds (%d integer)\n", std::pow(2.0f, (float)mmax), mmax);
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


/** get the limits and default value of a camera/stream property (exposure, zoom etc) */
bool PlatformStream::getPropertyLimits(CapPropertyID propID, int32_t *emin, int32_t *emax, int32_t *dValue)
{
    if ((m_camControl == nullptr) || (emin == nullptr) || (emax == nullptr))
    {
        return false;
    }

    if (propID < CAPPROPID_LAST)
    {
        long flags, mmin, mmax, delta, defaultValue;
        if (gs_properties[propID].isCameraControl)
        {
            // use Camera control
            if (m_camControl->GetRange(gs_properties[propID].dsProp,
                    &mmin, &mmax, &delta, &defaultValue, &flags) == S_OK)
            {   
                *emin = mmin;
                *emax = mmax;
                *dValue = defaultValue;
                return true;
            }            
        }
        else
        {
            // use VideoProcAmp
            if (m_videoProcAmp == nullptr)
            {
                return false; // no VideoProcAmp on board camera
            }

            if (m_videoProcAmp->GetRange(gs_properties[propID].dsProp, 
                &mmin, &mmax, &delta, &defaultValue, &flags) == S_OK)
            {   
                *emin = mmin;
                *emax = mmax;
                *dValue = defaultValue;
                return true;
            }
        }
    }

    return false;
}


/** set property (exposure, zoom etc) of camera/stream */
bool PlatformStream::setProperty(uint32_t propID, int32_t value)
{
    if (m_camControl == nullptr)
    {
        return false;
    }


    if (propID < CAPPROPID_LAST)
    {
        long flags, dummy;
        if (gs_properties[propID].isCameraControl)
        {
            // use Camera control
            // first we get the property so we can retain the flag settings
            if (m_camControl->Get(gs_properties[propID].dsProp, &dummy, &flags) != S_OK)
            {
                return false;
            }

            // now we set the property.
            if (m_camControl->Set(gs_properties[propID].dsProp, value, flags) != S_OK)
            {
                return false;
            }

            return true;
        }
        else
        {
            // use VideoProcAmp
            if (m_videoProcAmp == nullptr)
            {
                return false; // no VideoProcAmp on board camera
            }

            // first we get the property so we can retain the flag settings
            if (m_videoProcAmp->Get(gs_properties[propID].dsProp, &dummy, &flags) != S_OK)
            {
                return false;
            }

            // now we set the property.
            if (m_videoProcAmp->Set(gs_properties[propID].dsProp, value, flags) != S_OK)
            {
                return false;
            }

            return true;
        }
    }

    return false;
}


/** set automatic state of property (exposure, zoom etc) of camera/stream */
bool PlatformStream::setAutoProperty(uint32_t propID, bool enabled)
{
    if (m_camControl == 0)
    {
        return false;
    }

    long prop = 0;
    switch(propID)
    {
    case CAPPROPID_EXPOSURE:
        prop = CameraControl_Exposure;
        break;
    case CAPPROPID_FOCUS:
        prop = CameraControl_Focus;
        break;
    case CAPPROPID_ZOOM:
        prop = CameraControl_Zoom;        
        break;
    case CAPPROPID_WHITEBALANCE:
        prop = VideoProcAmp_WhiteBalance; 
        break;
    case CAPPROPID_GAIN:
        prop = VideoProcAmp_Gain; 
        break;        
    default:
        return false;
    }

    if ((propID != CAPPROPID_WHITEBALANCE) && (propID != CAPPROPID_GAIN))
    {
        //FIXME: check return codes.
        if (enabled)
            m_camControl->Set(prop, 0, CameraControl_Flags_Auto | KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE);
        else
            m_camControl->Set(prop, 0, CameraControl_Flags_Manual | KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE);
    }
    else
    {
        //note: m_videoProcAmp only exists if the camera
        //      supports hardware accelleration of 
        //      video frame processing, such as
        //      white balance etc.
        if (m_videoProcAmp == nullptr)
        {
            return false;
        }

        // get the current value so we can just set the auto flag
        // but leave the actualy setting itself intact.
        long currentValue, flags;
        if (FAILED(m_videoProcAmp->Get(prop, &currentValue, &flags)))
        {
            return false;
        }

        //FIXME: check return codes.
        if (enabled)
            m_videoProcAmp->Set(prop, currentValue, VideoProcAmp_Flags_Auto);
        else
            m_videoProcAmp->Set(prop, currentValue, VideoProcAmp_Flags_Manual);
    }

    return true;
}

bool PlatformStream::getDSProperty(uint32_t propID, long &value, long &flags)
{
    if (m_camControl == 0)
    {
        return false;
    }

    if (propID < CAPPROPID_LAST)
    {
        if (gs_properties[propID].isCameraControl)
        {
            // use Camera control 
            if (FAILED(m_camControl->Get(gs_properties[propID].dsProp, &value, &flags)))
            {
                return false;
            }
            return true;                   
        }
        else
        {
            //note: m_videoProcAmp only exists if the camera
            //      supports hardware accelleration of 
            //      video frame processing, such as
            //      white balance etc.
            if (m_videoProcAmp == nullptr)
            {
                return false;
            }

            // get the current value so we can just set the auto flag
            // but leave the actualy setting itself intact.
            if (FAILED(m_videoProcAmp->Get(gs_properties[propID].dsProp, &value, &flags)))
            {
                return false;
            }
            return true;
        }
    }
    return false;
}

/** get property (exposure, zoom etc) of camera/stream */
bool PlatformStream::getProperty(uint32_t propID, int32_t &outValue)
{
    // in keeping with the documentation, we assume long here.. 
    // the DS documentation does not specify the actual bit-width
    // for the vars, but we use 32-bit ints in the capture lib
    // so we convert to 32-bits and hope for the best.. 

    long value, flags;
    if (PlatformStream::getDSProperty(propID, value, flags))
    {
        outValue = value;
        return true;
    }
    return false;
}

/** get automatic state of property (exposure, zoom etc) of camera/stream */
bool PlatformStream::getAutoProperty(uint32_t propID, bool &enabled)
{
    // Here, we assume that 
    // CameraControl_Flags_Auto == VideoProcAmp_Flags_Auto
    // and
    // CameraControl_Flags_Manual == VideoProcAmp_Flags_Manual
    // to simplify the code.
    // We make sure this assumption is true via a static assert
    
    static_assert(CameraControl_Flags_Auto == VideoProcAmp_Flags_Auto, "Boolean flags dont match - code change needed!");
    //static_assert(CameraControl_Flags_Manual == VideoProcAmp_Flags_Manual, "Boolean flags dont match - code change needed!");

    //LOG(LOG_VERBOSE, "PlatformStream::getAutoProperty called\n");

    long value, flags;
    if (PlatformStream::getDSProperty(propID, value, flags))
    {        
        enabled = ((flags & CameraControl_Flags_Auto) != 0);
        return true;
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


HRESULT PlatformStream::AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister)
{
    IMoniker * pMoniker = NULL;
    IRunningObjectTable *pROT = NULL;

    if (FAILED(GetRunningObjectTable(0, &pROT))) 
    {
        LOG(LOG_DEBUG,"AddToRot failed to get running object table\n");
        return E_FAIL;
    }
    
    const size_t STRING_LENGTH = 256;

    WCHAR wsz[STRING_LENGTH];
 
    StringCchPrintfW(
        wsz, STRING_LENGTH, 
        L"FilterGraph %08x pid %08x", 
        (DWORD_PTR)pUnkGraph, 
        GetCurrentProcessId()
    );
    
    HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) 
    {
        hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph,
            pMoniker, pdwRegister);
        pMoniker->Release();
        LOG(LOG_DEBUG,"Graph registered in running object table\n", hr);
    }
    else
    {
        LOG(LOG_DEBUG,"AddToRot failed to register graph (HRESULT=%08X)\n", hr);
    }
    pROT->Release();
    
    return hr;
}


void PlatformStream::RemoveFromRot(DWORD pdwRegister)
{
    IRunningObjectTable *pROT;
    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
    {
        pROT->Revoke(pdwRegister);
        pROT->Release();
    }
}

HRESULT PlatformStream::SaveGraphFile(IGraphBuilder *pGraph)
{
    const WCHAR wszStreamName[] = L"ActiveMovieGraph"; 
    const WCHAR wszPath[] = L"filtergraph.grf";
    HRESULT hr;
    
    IStorage *pStorage = NULL;
    hr = StgCreateDocfile(
        wszPath,
        STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
        0, &pStorage);
    if(FAILED(hr)) 
    {
        return hr;
    }

    IStream *pStream;
    hr = pStorage->CreateStream(
        wszStreamName,
        STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
        0, 0, &pStream);
    if (FAILED(hr)) 
    {
        pStorage->Release();    
        return hr;
    }

    IPersistStream *pPersist = NULL;
    pGraph->QueryInterface(IID_IPersistStream, (void**)&pPersist);
    hr = pPersist->Save(pStream, TRUE);
    pStream->Release();
    pPersist->Release();
    if (SUCCEEDED(hr)) 
    {
        hr = pStorage->Commit(STGC_DEFAULT);
    }
    pStorage->Release();
    return hr;
}

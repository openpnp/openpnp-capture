/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Platform/implementation specific structures
    and typedefs.

    The platform classes are also responsible for converting
    the frames into 24-bit per pixel RGB frames.

*/

#include <vector>
#include <stdio.h>

#include "../common/logging.h"
#include "scopedcomptr.h"
#include "platformstream.h"
#include "platformcontext.h"

// a platform factory function needed by
// libmain.cpp
Context* createPlatformContext()
{
    return new PlatformContext();
}

PlatformContext::PlatformContext() :
    Context()
{
    HRESULT hr;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (hr != S_OK)
    {
        LOG(LOG_CRIT, "Context creation failed!\n");
        //FIXME: ok, what now? is it acceptable to throw an
        //       exception here?
    }
    else
    {
        LOG(LOG_DEBUG, "Context created\n");
    }

    enumerateDevices();
}

PlatformContext::~PlatformContext()
{
    CoUninitialize();
}

#if 1
bool PlatformContext::enumerateDevices()
{
    ICreateDevEnum*		dev_enum = nullptr;
	IEnumMoniker*		enum_moniker = nullptr;
	IMoniker*			moniker = nullptr;
	IPropertyBag*		pbag = nullptr;

    LOG(LOG_DEBUG, "Enumerating devices\n");

    m_devices.clear();

	//create an enumerator for video input devices
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,CLSCTX_INPROC_SERVER,IID_ICreateDevEnum,(void**) &dev_enum);
	if ((hr != S_OK) || (dev_enum == nullptr))
    {
        LOG(LOG_CRIT, "Could not create ICreateDevEnum object\n");
        return false;
    }
    else
    {
        LOG(LOG_DEBUG, "ICreateDevEnum created\n");
    }

    ScopedComPtr<ICreateDevEnum> devEnum(dev_enum);

	hr = devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&enum_moniker,NULL);
	if (hr == S_FALSE)
    {
        // no devices found!
        LOG(LOG_INFO, "No devices found\n");
        return true;
    }
    if (hr != S_OK)
    {
        LOG(LOG_CRIT, "Could not create class enumerator object\n");
        return false;
    }

    ScopedComPtr<IEnumMoniker> enumMoniker(enum_moniker);

	//get devices
	uint32_t num_devices = 0;
    VARIANT name;
	while (enumMoniker->Next(1, &moniker,0) == S_OK)
	{
		//get properties
		hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**) &pbag);
        ScopedComPtr<IPropertyBag> propertyBag(pbag);

		if (hr >= 0)
		{            
            platformDeviceInfo *info = new platformDeviceInfo();
			VariantInit(&name);

			//get the description
			hr = propertyBag->Read(L"Description", &name, 0);
			if (hr < 0) hr = propertyBag->Read(L"FriendlyName", &name, 0);
			if (hr >= 0)
			{
				BSTR BStringPtr = name.bstrVal;

                if (BStringPtr)
                {
                    // copy wchar device name into info structure so we can reference the
                    // device later
                    info->m_filterName = std::wstring(BStringPtr);

                    // convert wchar string to UTF-8 to pass to JAVA                    
                    info->m_name = wcharPtrToString(BStringPtr);
                }                
            }
            else
            {
                LOG(LOG_ERR, "Could not generate device name for device!\n");
            }

            hr = propertyBag->Read(L"DevicePath", &name, 0);
			if (hr >= 0)
			{
        		BSTR BStringPtr = name.bstrVal;            
                if (BStringPtr)
                {
                    info->m_devicePath = std::wstring(BStringPtr);
                    LOG(LOG_INFO, "     -> PATH %s\n", wcharPtrToString(BStringPtr).c_str());                    
                } 
            }

            enumerateFrameInfo(moniker, info);

            moniker->AddRef();
            info->m_moniker = moniker;
            m_devices.push_back(info);
            LOG(LOG_INFO, "ID %d -> %s\n", num_devices, info->m_name.c_str());            
        }

        num_devices++;
    }
    return true;
}
#else


std::wstring PlatformContext::AttributeGetString(IMFAttributes *pAttributes, REFGUID guidkey)
{
    HRESULT hr = S_OK;
    UINT32 cchLength = 0;
    WCHAR *pString = nullptr;
    std::wstring retString;

    hr = pAttributes->GetStringLength(guidkey, &cchLength);
    
    if (SUCCEEDED(hr))
    {
        pString = new WCHAR[cchLength + 1];
        if (pString == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pAttributes->GetString(
            guidkey, pString, cchLength + 1, &cchLength);

        retString = std::wstring(pString);
    }

    if (pString)
    {
        delete [] pString;
    }
    return retString;
}

HRESULT EnumerateCaptureFormats(IMFMediaSource *pSource)
{
    IMFPresentationDescriptor *pPD = NULL;
    IMFStreamDescriptor *pSD = NULL;
    IMFMediaTypeHandler *pHandler = NULL;
    IMFMediaType *pType = NULL;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (FAILED(hr))
    {
        goto done;
    }

    DWORD indexCount = 0;

    hr = pPD->GetStreamDescriptorCount(&indexCount);
    if (FAILED(hr))
    {
        goto done;
    }

    LOG(LOG_DEBUG, "Threre are %d stream descriptors\n", indexCount);

    for(uint32_t idx=0; idx<indexCount; idx++)
    {
        BOOL fSelected;
        hr = pPD->GetStreamDescriptorByIndex(idx, &fSelected, &pSD);
        if (FAILED(hr))
        {
            goto done;
        }

        // skip non-selected indeces.. 
        // Note: no idea what the unselected
        //       stuff is for .. ?!?
        //if (!fSelected)
        //{
        //    SafeRelease(&pSD);
        //    continue;
        //}

        LOG(LOG_DEBUG, "  Descriptor %d (selected=%d)\n", idx, fSelected);

        hr = pSD->GetMediaTypeHandler(&pHandler);
        if (FAILED(hr))
        {
            goto done;
        }

        DWORD cTypes = 0;
        hr = pHandler->GetMediaTypeCount(&cTypes);
        if (FAILED(hr))
        {
            goto done;
        }

        LOG(LOG_INFO,"  %d media types\n", cTypes);

        for (DWORD i = 0; i < cTypes; i++)
        {
            pType = nullptr;

            hr = pHandler->GetMediaTypeByIndex(i, &pType);
            if (FAILED(hr))
            {
                goto done;
            }

            AM_MEDIA_TYPE *myFormat = nullptr;
            hr = pType->GetRepresentation(FORMAT_MFVideoFormat, (void**)&myFormat);
            if (SUCCEEDED(hr))
            {
                MFVIDEOFORMAT *mfvFormat = reinterpret_cast<MFVIDEOFORMAT *>(myFormat->pbFormat);
                if (mfvFormat->dwSize != sizeof(MFVIDEOFORMAT))
                {
                    LOG(LOG_WARNING, "MFVideoFormat is not the correct size!\n");
                }

                std::string formatType;         
                if (IsEqualGUID(mfvFormat->guidFormat, MFVideoFormat_MJPG))
                {
                    formatType = "MJPG";
                }
                else if (IsEqualGUID(mfvFormat->guidFormat, MFVideoFormat_NV12))
                {
                    formatType = "NV12";
                }
                else if (IsEqualGUID(mfvFormat->guidFormat, MFVideoFormat_YUY2))
                {
                    formatType = "YUY2";
                }
                else
                {
                    GUID guid = mfvFormat->guidFormat;
                    char guidstr[100];
                    sprintf(guidstr, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
                        guid.Data1, guid.Data2, guid.Data3, 
                        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

                    formatType = std::string(guidstr);
                }

                LOG(LOG_INFO, "  %d x %d  %d fps ", 
                    mfvFormat->videoInfo.dwWidth, 
                    mfvFormat->videoInfo.dwHeight,
                    mfvFormat->videoInfo.FramesPerSecond);

                LOG(LOG_INFO, "%s\n", formatType.c_str());
                pType->FreeRepresentation(FORMAT_MFVideoFormat, myFormat);
            }
            else
            {
                goto done;
            }

            SafeRelease(&pType);
        }
        SafeRelease(&pSD);        
        SafeRelease(&pHandler);
    } // index loop

done:
    SafeRelease(&pPD);
    SafeRelease(&pSD);
    SafeRelease(&pHandler);    
    SafeRelease(&pType);
    return hr;
}

// using Media Foundation API
bool PlatformContext::enumerateDevices()
{
    IMFMediaSource *pSource = nullptr;
    IMFAttributes *pAttributes = nullptr;
    IMFActivate **ppDevices = nullptr;

    // Create an attribute store to specify the enumeration parameters.
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr))
    {
        goto done;
    }

    // Source type: video capture devices
    hr = pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, 
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
    if (FAILED(hr))
    {
        goto done;
    }

    // Enumerate devices.
    UINT32 count;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr))
    {
        goto done;
    }

    LOG(LOG_DEBUG, "Found %d capture devices.\n", count);

    if (count == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    for(uint32_t i=0; i<count; i++)
    {
        // MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME
        // MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK
        uint32_t strLen = 0;
        HRESULT hr = ppDevices[i]->GetStringLength(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &strLen);
        if (SUCCEEDED(hr))
        {
            platformDeviceInfo *info = new platformDeviceInfo();
            info->m_name = wstringToString(AttributeGetString(ppDevices[i], MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME));
            info->m_devicePath = AttributeGetString(ppDevices[i], MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);
            info->m_moniker = 0;
            LOG(LOG_INFO, "device: %s\n", info->m_name.c_str());
            LOG(LOG_INFO, "        %s\n", wstringToString(info->m_devicePath).c_str());
            m_devices.push_back(info);

            hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
            if (FAILED(hr))
            {
                LOG(LOG_ERR, "Failed to activate MF object (HRESULT=%08X)\n", hr);
                goto done;
            }

            EnumerateCaptureFormats(pSource);
            SafeRelease(&pSource);
        }
    }

#if 0
    // Create the media source object.
    hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
    if (FAILED(hr))
    {
        goto done;
    }

    *ppSource = pSource;
    (*ppSource)->AddRef();
#endif
done:
    SafeRelease(&pAttributes);

    for (DWORD i = 0; i < count; i++)
    {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);
    SafeRelease(&pSource);
    return true;
}

#endif


std::string PlatformContext::wstringToString(const std::wstring &wstr)
{
    return wcharPtrToString(wstr.c_str());
}

std::string PlatformContext::wcharPtrToString(const wchar_t *sstr)
{
    std::vector<char> buffer;
    int32_t chars = WideCharToMultiByte(CP_UTF8, 0, sstr, -1, nullptr, 0, nullptr, nullptr);    
    if (chars == 0) return std::string("");

    buffer.resize(chars);
    WideCharToMultiByte(CP_UTF8, 0, sstr, -1, &buffer[0], chars, nullptr, nullptr);
    return std::string(&buffer[0]);
} 



// Release the format block for a media type.

void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL)
    {
        // pUnk should not be used.
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

// Delete a media type structure that was allocated on the heap.
void _DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt != NULL)
    {
        _FreeMediaType(*pmt); 
        CoTaskMemFree(pmt);
    }
}

bool PinMatchesCategory(IPin *pPin, REFGUID Category)
{
    bool bFound = FALSE;

    IKsPropertySet *pKs = NULL;
    HRESULT hr = pPin->QueryInterface(IID_PPV_ARGS(&pKs));
    if (SUCCEEDED(hr))
    {
        GUID PinCategory;
        DWORD cbReturned;
        hr = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, 
            &PinCategory, sizeof(GUID), &cbReturned);
        if (SUCCEEDED(hr) && (cbReturned == sizeof(GUID)))
        {
            bFound = (PinCategory == Category);
        }
        pKs->Release();
    }
    return bFound;
}

HRESULT FindPinByCategory(
    IBaseFilter *pFilter, // Pointer to the filter to search.
    PIN_DIRECTION PinDir, // Direction of the pin.
    REFGUID Category,     // Pin category.
    IPin **ppPin)         // Receives a pointer to the pin.
{
    *ppPin = 0;

    HRESULT hr = S_OK;
    BOOL bFound = FALSE;

    IEnumPins *pEnum = 0;
    IPin *pPin = 0;

    hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }

    ScopedComPtr<IEnumPins> pinEnum(pEnum);

    while (hr = pinEnum->Next(1, &pPin, 0), hr == S_OK)
    {
        PIN_DIRECTION ThisPinDir;
        hr = pPin->QueryDirection(&ThisPinDir);
        if (FAILED(hr))
        {
            return hr;
        }

        ScopedComPtr<IPin> myPin(pPin);

        if ((ThisPinDir == PinDir) && 
            PinMatchesCategory(pPin, Category))
        {
            *ppPin = pPin;
            (*ppPin)->AddRef();   // The caller must release the interface.
            return S_OK;
        }
    }

    return E_FAIL;
}

bool PlatformContext::enumerateFrameInfo(IMoniker *moniker, platformDeviceInfo *info)
{
    IBaseFilter *pCap  = NULL;
    IEnumPins   *pEnum = NULL;
    IPin        *pPin  = NULL;

    LOG(LOG_DEBUG, "enumerateFrameInfo() called\n");

    HRESULT hr = moniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pCap);
    if (!SUCCEEDED(hr))
    {
        LOG(LOG_ERR, "No frame information: BindToObject failed.\n");
        return false;
    }

    ScopedComPtr<IBaseFilter> baseFilter(pCap);

    hr = baseFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        LOG(LOG_ERR, "No frame information: EnumPins failed.\n");
        return false;
    }

    if (FindPinByCategory(pCap, PINDIR_OUTPUT, PIN_CATEGORY_CAPTURE, &pPin) == S_OK)
    {
        LOG(LOG_INFO, "Capture pin found!\n");
    }
    else
    {
        LOG(LOG_ERR, "Could not find capture pin!\n");
        return false;
    }

    ScopedComPtr<IPin> capturePin(pPin);

    // retrieve an IAMStreamConfig interface
    IAMStreamConfig *pConfig = NULL;
    if (capturePin->QueryInterface(IID_IAMStreamConfig, (void**)&pConfig) != S_OK)
    {
        LOG(LOG_ERR, "Could not create IAMStreamConfig interface!\n");
        return false;
    }

    ScopedComPtr<IAMStreamConfig> streamConfig(pConfig);

    int iCount = 0, iSize = 0;
    hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);

    LOG(LOG_INFO,"Stream has %d capabilities.\n", iCount);

    // Check the size to make sure we pass in the correct structure.
    if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
    {
        // Use the video capabilities structure.

        for (int32_t iFormat = 0; iFormat < iCount; iFormat++)
        {
            VIDEO_STREAM_CONFIG_CAPS scc;
            AM_MEDIA_TYPE *pmtConfig;
            hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
            if (SUCCEEDED(hr))
            {
                /* Examine the format, and possibly use it. */
                if (pmtConfig->formattype == FORMAT_VideoInfo)
                {
                    // Check the buffer size.
                    if (pmtConfig->cbFormat >= sizeof(VIDEOINFOHEADER))
                    {
                        VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pmtConfig->pbFormat);
                        CapFormatInfo newFrameInfo;
                        if (pVih != nullptr)
                        {
                            newFrameInfo.bpp = pVih->bmiHeader.biBitCount;
                            if (pVih->bmiHeader.biCompression == BI_RGB)
                            {
                                newFrameInfo.fourcc = 'RGB ';
                            }
                            else if (pVih->bmiHeader.biCompression == BI_BITFIELDS)
                            {
                                newFrameInfo.fourcc = '   ';
                            }
                            else
                            {
                                newFrameInfo.fourcc = pVih->bmiHeader.biCompression;
                            }

                            newFrameInfo.width  = pVih->bmiHeader.biWidth;
                            newFrameInfo.height = pVih->bmiHeader.biHeight;
                            
                            if (pVih->AvgTimePerFrame != 0)
                            {
                                // pVih->AvgTimePerFrame is in units of 100ns
                                newFrameInfo.fps = static_cast<uint32_t>(10.0e6f/static_cast<float>(pVih->AvgTimePerFrame));
                            }
                            else
                            {
                                newFrameInfo.fps = 0;
                            }
                            
                            std::string fourCCString = fourCCToString(newFrameInfo.fourcc);

                            LOG(LOG_INFO, "%d x %d  %d fps  %d bpp FOURCC=%s\n", newFrameInfo.width, newFrameInfo.height, 
                                newFrameInfo.fps, newFrameInfo.bpp, fourCCString.c_str());

                            info->m_formats.push_back(newFrameInfo);
                        }
                    }
                }
                // Delete the media type when you are done.
                _DeleteMediaType(pmtConfig);
            }
        }
    }

    return true;
}

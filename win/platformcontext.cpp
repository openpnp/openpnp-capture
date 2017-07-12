/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Platform/implementation specific structures
    and typedefs.

*/

#include <vector>
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
        LOG(LOG_INFO, "Could not find capture pin!\n");
        return false;
    }

    

#if 0
    ScopedComPtr<IEnumPins> enumPins(pEnum);

    while(enumPins->Next(1, &pPin, 0) == S_OK)
    {
        ScopedComPtr<IPin> pin(pPin);

        PIN_INFO pinInfo;
        hr = pin->QueryPinInfo(&pinInfo);
        if (FAILED(hr))
        {
            LOG(LOG_ERR, "No frame information: QueryPinInfo failed.\n");
            return false;
        }

        pinInfo.

        PIN_DIRECTION PinDirThis;
        hr = pin->QueryDirection(&PinDirThis);
        if (FAILED(hr))
        {
            return hr;
        }
        if (PinDir == PinDirThis)
        {
            // Found a match. Return the IPin pointer to the caller.
            *ppPin = pPin;
            pEnum->Release();
            return S_OK;
        }
        // Release the pin for the next time through the loop.
        pPin->Release();


    }
        #endif    
    return true;
}
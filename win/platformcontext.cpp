/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform/implementation specific structures and typedefs.

    The platform classes are also responsible for converting
    the frames into 24-bit per pixel RGB frames.

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

#include <vector>
#include <stdio.h>

#include <windows.h>
#include <mmsystem.h> // for MAKEFOURCC macro

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

PlatformContext::PlatformContext() : Context()
{
    HRESULT hr;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (hr != S_OK)
    {
        // This might happen when another part of the program
        // as already called CoInitializeEx.
        // and we can carry on without problems... 
        LOG(LOG_WARNING, "PlatformContext::CoInitializeEx failed (HRESULT = %08X)!\n", hr);
    }
    else
    {
        LOG(LOG_DEBUG, "PlatformContext created\n");
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

	hr = devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&enum_moniker, 0);
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

		if (hr >= 0)
		{            
            platformDeviceInfo *info = new platformDeviceInfo();
			VariantInit(&name);

			//get the description
			hr = pbag->Read(L"Description", &name, 0);
			if (hr < 0) hr = pbag->Read(L"FriendlyName", &name, 0);
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
                    info->m_uniqueID = info->m_name;
                }                
            }
            else
            {
                LOG(LOG_ERR, "Could not generate device name for device!\n");
            }

            hr = pbag->Read(L"DevicePath", &name, 0);
			if (hr >= 0)
			{
        		BSTR BStringPtr = name.bstrVal;            
                if (BStringPtr)
                {
                    info->m_devicePath = std::wstring(BStringPtr);
                    info->m_uniqueID.append(" ");
                    info->m_uniqueID.append(wcharPtrToString(BStringPtr));
                    LOG(LOG_INFO, "     -> PATH %s\n", wcharPtrToString(BStringPtr).c_str());
                } 
                else
                {
                    LOG(LOG_WARNING, "     device path not found!");  
                }
            }

            enumerateFrameInfo(moniker, info);
            m_devices.push_back(info);

            LOG(LOG_INFO, "ID %d -> %s\n", num_devices, info->m_name.c_str());

            VariantClear(&name);

            pbag->Release();
            pbag = nullptr;

            moniker->Release();
            moniker = nullptr;

            num_devices++; 
        }
        else{
            moniker->Release();
        }
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
                                newFrameInfo.fourcc = MAKEFOURCC('R', 'G', 'B', ' ');
                            }
                            else if (pVih->bmiHeader.biCompression == BI_BITFIELDS)
                            {
                                newFrameInfo.fourcc = MAKEFOURCC(' ', ' ', ' ', ' ');
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


HRESULT FindCaptureDevice(IBaseFilter** ppSrcFilter, const wchar_t* devicePath)
{
    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;
    IMoniker* pMoniker = nullptr;
    IPropertyBag* pbag = nullptr;
    HRESULT hr = S_OK;

    std::wstring strDevicePath = devicePath;

    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (FAILED(hr))
        return hr;

    ScopedComPtr<ICreateDevEnum> devEnum(pDevEnum);

    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);

    ScopedComPtr<IEnumMoniker> enumMoniker(pEnum);
    if (hr == S_OK) {

        while (enumMoniker->Next(1, &pMoniker, 0) == S_OK)
        {
            hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pbag));
            ScopedComPtr<IPropertyBag> propertyBag(pbag);

            if (FAILED(hr)) {
                pMoniker->Release();
                continue;  // Skip this one, maybe the next one will work.
            }

            bool bMatch = false;

            VARIANT varName;
            VariantInit(&varName);
            hr = pbag->Read(L"DevicePath", &varName, 0);

            if (SUCCEEDED(hr)) {
                if (strDevicePath == varName.bstrVal)
                    bMatch = true;
            }
            VariantClear(&varName);

            if (bMatch)
            {
                hr = pMoniker->BindToObject(0, 0, IID_PPV_ARGS(ppSrcFilter));
                return hr;
            }
        }
    }

    return E_FAIL;
}


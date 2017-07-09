/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Platform/implementation specific structures
    and typedefs.

*/

//#include <Mfidl.h>
//#include <Mfapi.h>

#include <vector>
#include "context.h"
#include "logging.h"
#include "scopedcomptr.h"
#include "stream.h"

Context::Context()
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

#if 0
	//create the FilterGraph
	hr = CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,IID_IFilterGraph2,(void**) &graph);
	if (hr < 0) throw hr; 
	
	//create the CaptureGraphBuilder
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2,NULL,CLSCTX_INPROC_SERVER,IID_ICaptureGraphBuilder2,(void**) &capture);
	if (hr < 0) throw hr;
	
	//get the controller for the graph
	hr = graph->QueryInterface(IID_IMediaControl, (void**) &control);
	if (hr < 0) throw hr;

	capture->SetFiltergraph(graph);
#endif
}

Context::~Context()
{
    CoUninitialize();
    LOG(LOG_DEBUG, "Context destroyed\n");
}

const char* Context::getDeviceName(CapDeviceID id) const
{
    if (id >= m_devices.size())
    {
        return NULL; // no such device ID!
    }
    return m_devices[id].m_name.c_str();
}

uint32_t Context::getDeviceCount() const
{
    return m_devices.size();
}

bool Context::enumerateDevices()
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
            deviceInfo info;
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
                    info.m_filterName = std::wstring(BStringPtr);

                    // convert wchar string to UTF-8 to pass to JAVA                    
                    info.m_name = wcharPtrToString(BStringPtr);
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
                    info.m_devicePath = std::wstring(BStringPtr);
                    LOG(LOG_INFO, "     -> PATH %s\n", wcharPtrToString(BStringPtr).c_str());                    
                } 
            }

            moniker->AddRef();
            info.m_moniker = moniker;
            m_devices.push_back(info);
            LOG(LOG_INFO, "ID %d -> %s\n", num_devices, info.m_name.c_str());            
        }

        #if 0
        // try display name.. 
        LPOLESTR dname;
        hr = moniker->GetDisplayName(0,0, &dname);
        if (hr == S_OK)
        {
            int32_t chars = WideCharToMultiByte(CP_UTF8, 0, dname, -1, nullptr, 0, nullptr, nullptr);
            std::vector<char> buffer;
            buffer.resize(chars);
            WideCharToMultiByte(CP_UTF8, 0, dname, -1, &buffer[0], chars, nullptr, nullptr);
            LOG(LOG_INFO, "     -> DNAME %s\n", &buffer[0]);
        }
        #endif

        num_devices++;
    }
    return true;
}

#if 0

// save this code for when we want to experiment further with AM

void Context::AMTest()
{
    UINT32 count = 0;

    IMFAttributes *pConfig = NULL;
    IMFActivate **ppDevices = NULL;

    // Create an attribute store to hold the search criteria.
    HRESULT hr = MFCreateAttributes(&pConfig, 1);

    // Request video capture devices.
    if (SUCCEEDED(hr))
    {
        hr = pConfig->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, 
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
            );
    }

    // Enumerate the devices,
    if (SUCCEEDED(hr))
    {
        hr = MFEnumDeviceSources(pConfig, &ppDevices, &count);
    }

    LOG(LOG_INFO, "Media Foundation found %d devices\n", count);

    for(uint32_t i=0; i<count; i++)
    {
        // https://msdn.microsoft.com/en-us/library/windows/desktop/bb970331(v=vs.85).aspx
        uint32_t count;
        PROPVARIANT pvar;
        GUID myGUID;
        hr = ppDevices[i]->LockStore();
        hr = ppDevices[i]->GetCount(&count);
        for(uint32_t j=0; j<count; j++)
        {
            ppDevices[i]->GetItemByIndex(j, &myGUID, &pvar);
            
            switch(pvar.vt)
            {
            case VT_LPWSTR:
                LOG(LOG_INFO, "%s\n", wcharPtrToString(pvar.pwszVal).c_str());
                break;
            case VT_CLSID: 
                LOG(LOG_INFO, "VARTYPE: %d (CLSID/GUID)\n", pvar.vt);
                break;
            case VT_UI4:
                LOG(LOG_INFO, "VARTYPE: %d (UINT32) 0x%08X\n", pvar.vt, pvar.ulVal);
                break;
            default:
                LOG(LOG_INFO, "VARTYPE: %d\n", pvar.vt);
            }

            PropVariantClear(&pvar);
        }
    }

    #if 0
    // Create a media source for the first device in the list.
    if (SUCCEEDED(hr))
    {
        if (count > 0)
        {
            hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(ppSource));
        }
        else
        {
            hr = MF_E_NOT_FOUND;
        }
    }
    #endif

    for (DWORD i = 0; i < count; i++)
    {
        ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
    return;
}

#endif

std::string Context::wstringToString(const std::wstring &wstr)
{
    return wcharPtrToString(wstr.c_str());
}

std::string Context::wcharPtrToString(const wchar_t *sstr)
{
    std::vector<char> buffer;
    int32_t chars = WideCharToMultiByte(CP_UTF8, 0, sstr, -1, nullptr, 0, nullptr, nullptr);    
    if (chars == 0) return std::string("");

    buffer.resize(chars);
    WideCharToMultiByte(CP_UTF8, 0, sstr, -1, &buffer[0], chars, nullptr, nullptr);
    return std::string(&buffer[0]);
} 


int32_t Context::openStream(deviceInfo *device)
{
    if (device == 0)
    {
        if (m_devices.size() > 0)
        {
            device = &m_devices[0];
        }
        else
        {
            LOG(LOG_ERR, "openStream: No devices found\n", device->m_name.c_str());
            return -1;
        }
    }

    Stream *s = new Stream();
    if (!s->open(this, device, 0,0,0))
    {
        LOG(LOG_ERR, "Could not open stream for device %s\n", device->m_name.c_str());
        return -1;
    }
    m_streams.push_back(s);
    return m_streams.size()-1;
}

void Context::closeStream(int32_t streamID)
{
    if (streamID < 0)
    {
        LOG(LOG_ERR, "closeStream was called with a negative stream ID\n");
        return;
    }
    if (static_cast<uint32_t>(streamID) >= m_streams.size())
    {
        LOG(LOG_ERR, "closeStream was called with an out-of-bounds stream ID\n");
        return;        
    }
    if (m_streams[streamID] == nullptr)
    {
        LOG(LOG_ERR, "closeStream was called on a closed stream\n");
        return;
    }
    else
    {
        delete m_streams[streamID];
        m_streams[streamID] = nullptr;
    }
}

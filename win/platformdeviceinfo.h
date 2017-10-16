/*

    OpenPnp-Capture: a video capture subsystem.

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

#ifndef win_platformdeviceinfo_h
#define win_platformdeviceinfo_h

#include <windows.h>
#include <Dshow.h>
#include <string>

#include "../common/deviceinfo.h"

/** device information struct/object */
class platformDeviceInfo : public deviceInfo
{
public:
    platformDeviceInfo() : deviceInfo(), m_moniker(0) {}

    ~platformDeviceInfo()
    {
        if (m_moniker != nullptr)
        {
            //FIXME: not sure what to do with
            // IMoniker* here. When I call
            // ->Release(), the program crashes ?
            // even an additional AddRef was
            // applied.. ? Documentation unclear.
        }
    }

    std::wstring    m_filterName;   ///< DirectShow internal device name
    std::wstring    m_devicePath;   ///< unique device path
    IMoniker*       m_moniker;      ///< DirectShow object for capture device
};

#endif
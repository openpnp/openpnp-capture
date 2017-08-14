package org.openpnp.capture;

import org.openpnp.capture.library.CapFormatInfo;
import org.openpnp.capture.library.OpenpnpCaptureLibrary;

import com.sun.jna.Pointer;

public class OpenPnpCapture {
    // Library prefixes
    // darwin
    // win32-x86
    // win32-x86-64
    // linux-x86
    // linux-x86-64
    public static void main(String[] args) {
        OpenPnpCapture capture = new OpenPnpCapture();
        Pointer context = OpenpnpCaptureLibrary.INSTANCE.Cap_createContext();
        int deviceCount = OpenpnpCaptureLibrary.INSTANCE.Cap_getDeviceCount(context);
        for (int deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
            String deviceName = OpenpnpCaptureLibrary.INSTANCE.Cap_getDeviceName(context, deviceIndex).getString(0);
            System.out.println(String.format("%d %s", deviceIndex, deviceName));
            int formatCount = OpenpnpCaptureLibrary.INSTANCE.Cap_getNumFormats(context, deviceIndex);
            for (int formatIndex = 0; formatIndex < formatCount; formatIndex++) {
                CapFormatInfo formatInfo = new CapFormatInfo();
                OpenpnpCaptureLibrary.INSTANCE.Cap_getFormatInfo(context, deviceIndex, formatIndex, formatInfo);
                System.out.println(String.format("  %dx%d @ %d fps", formatInfo.width, formatInfo.height, formatInfo.fps));
            }
        }
    }
}

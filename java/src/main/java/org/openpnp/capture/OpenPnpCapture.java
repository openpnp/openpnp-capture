package org.openpnp.capture;

import org.openpnp.capture.library.OpenpnpCaptureLibrary;

import com.sun.jna.Native;
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
        for (int i = 0; i < deviceCount; i++) {
            String deviceName = OpenpnpCaptureLibrary.INSTANCE.Cap_getDeviceName(context, i).getString(0);
            System.out.println(String.format("%d %s", i, deviceName));
        }
    }
}

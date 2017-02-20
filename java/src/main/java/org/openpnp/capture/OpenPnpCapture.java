package org.openpnp.capture;

import java.util.ArrayList;
import java.util.List;

import com.sun.jna.Pointer;

public class OpenPnpCapture {
    Pointer context;

    public OpenPnpCapture() {
        context = OpenPnpCaptureLibrary.INSTANCE.create_context();
    }

    @Override
    protected void finalize() throws Throwable {
        release();
    }
    
    public void release() {
        if (context != null) {
            OpenPnpCaptureLibrary.INSTANCE.release_context(context);
            context = null;
        }
    }

    public List<CaptureDevice> listDevices() {
        List<CaptureDevice> devices = new ArrayList<>();
        Pointer pDevices = OpenPnpCaptureLibrary.INSTANCE.list_devices(context);
        Pointer[] paDevices = pDevices.getPointerArray(0);
        for (Pointer p : paDevices) {
            CaptureDevice device = new CaptureDevice(p);
            device.read();
            devices.add(device);
        }
        return devices;
    }

    public static void main(String[] args) {
        OpenPnpCapture capture = new OpenPnpCapture();
        for (CaptureDevice device : capture.listDevices()) {
            System.out.println(device);
        }
        capture = null;
        System.gc();
        System.runFinalization();
    }
}

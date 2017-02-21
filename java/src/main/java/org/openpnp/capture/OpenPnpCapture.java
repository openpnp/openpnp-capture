package org.openpnp.capture;

import java.util.Arrays;
import java.util.List;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;

public class OpenPnpCapture {
    Pointer context;

    public OpenPnpCapture() {
        PointerByReference context = new PointerByReference();
        OpenPnpCaptureLibrary.INSTANCE.create_context(context);
        this.context = context.getValue();
    }

    public void release() {
        if (context != null) {
            OpenPnpCaptureLibrary.INSTANCE.release_context(context);
            context = null;
        }
    }

    public List<CaptureDevice> listDevices() {
        PointerByReference pDevices = new PointerByReference();
        IntByReference devicesLength = new IntByReference();
        OpenPnpCaptureLibrary.INSTANCE.list_devices(context, pDevices, devicesLength);
        CaptureDevice device = new CaptureDevice(pDevices.getValue());
        device.read();
        CaptureDevice[] devices = (CaptureDevice[]) device.toArray(devicesLength.getValue());
        return Arrays.asList(devices);
    }

    public static void main(String[] args) {
        OpenPnpCapture capture = new OpenPnpCapture();
        for (CaptureDevice device : capture.listDevices()) {
            System.out.println(device);
            for (CaptureFormat format : device.getFormats()) {
                System.out.println("    " + format);
            }
        }
        capture.release();
    }
}

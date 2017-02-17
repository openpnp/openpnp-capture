package org.openpnp.capture;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;

public class OpenPnpCapture {
    public interface OpenPnpCaptureLibrary extends Library {
        // Library prefixes
        // darwin
        // win32-x86
        // win32-x86-64
        // linux-x86
        // linux-x86-64

        OpenPnpCaptureLibrary INSTANCE = (OpenPnpCaptureLibrary) Native
                .loadLibrary("openpnp-capture", OpenPnpCaptureLibrary.class);

        Pointer create_context();
        Pointer[] list_devices(Pointer context);
        void release_context(Pointer context);
    }

    public static void main(String[] args) {
        Pointer context = OpenPnpCaptureLibrary.INSTANCE.create_context();
        for (Pointer p : OpenPnpCaptureLibrary.INSTANCE.list_devices(context)) {
            System.out.println(p);
        }
        OpenPnpCaptureLibrary.INSTANCE.release_context(context);
    }
}

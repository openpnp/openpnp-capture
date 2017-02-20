package org.openpnp.capture;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;

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
    Pointer list_devices(Pointer context);
    void release_context(Pointer context);
}
package org.openpnp.capture;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;

public interface OpenPnpCaptureLibrary extends Library {
    // Library prefixes
    // darwin
    // win32-x86
    // win32-x86-64
    // linux-x86
    // linux-x86-64
    OpenPnpCaptureLibrary INSTANCE = (OpenPnpCaptureLibrary) Native
            .loadLibrary("openpnp-capture", OpenPnpCaptureLibrary.class);
    
    /**
     * <i>native declaration : /Users/jason/Projects/openpnp/openpnp-capture/include/openpnp-capture.h</i><br>
     * enum values
     */
    public static interface capture_status {
        /** <i>native declaration : /Users/jason/Projects/openpnp/openpnp-capture/include/openpnp-capture.h:5</i> */
        public static final int CAPTURE_OK = 0;
        /** <i>native declaration : /Users/jason/Projects/openpnp/openpnp-capture/include/openpnp-capture.h:6</i> */
        public static final int CAPTURE_ERROR = -1;
    };
    /**
     * Original signature : <code>capture_status create_context(capture_context**)</code><br>
     * <i>native declaration : /Users/jason/Projects/openpnp/openpnp-capture/include/openpnp-capture.h:27</i>
     */
    int create_context(PointerByReference context);
    /**
     * Original signature : <code>capture_status release_context(capture_context*)</code><br>
     * <i>native declaration : /Users/jason/Projects/openpnp/openpnp-capture/include/openpnp-capture.h:29</i>
     */
    int release_context(Pointer context);
    /**
     * Original signature : <code>capture_status list_devices(capture_context*, capture_device**, unsigned int*)</code><br>
     * <i>native declaration : /Users/jason/Projects/openpnp/openpnp-capture/include/openpnp-capture.h:31</i>
     */
    int list_devices(Pointer context, PointerByReference devices, IntByReference devices_length);
}
package org.openpnp.capture;

import java.util.ArrayList;
import java.util.List;

import org.openpnp.capture.library.CapFormatInfo;
import org.openpnp.capture.library.OpenpnpCaptureLibrary;

import com.sun.jna.Pointer;

public class CaptureDevice {
    Pointer context;
    int index;
    String name;
    List<CapFormatInfo> formats = new ArrayList<>();
    
    public CaptureDevice(Pointer context, int index) {
        this.context = context;
        this.index = index;
        this.name = OpenpnpCaptureLibrary.INSTANCE.Cap_getDeviceName(context, index).getString(0, "UTF8");
        int formatCount = OpenpnpCaptureLibrary.INSTANCE.Cap_getNumFormats(context, index);
        for (int formatIndex = 0; formatIndex < formatCount; formatIndex++) {
            CapFormatInfo formatInfo = new CapFormatInfo();
            OpenpnpCaptureLibrary.INSTANCE.Cap_getFormatInfo(context, index, formatIndex, formatInfo);
            formats.add(formatInfo);
        }
    }
    
    @Override
    public String toString() {
        return name;
    }
}

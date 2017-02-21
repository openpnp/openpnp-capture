package org.openpnp.capture;
import java.util.Arrays;
import java.util.List;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;
public class CaptureDevice extends Structure {
	public Pointer name;
	public Pointer unique_id;
	public Pointer manufacturer;
	public Pointer model;
    public byte supportsExposureAuto;
    public byte supportsExposureManual;
    public byte supportsFocusAuto;
    public byte supportsFocusManual;
    public CaptureFormat.ByReference formats;
    public int formats_length;
    public Pointer _internal;
	
	public CaptureDevice() {
	    super();
	}
	
	public CaptureDevice(Pointer peer) {
	    super(peer);
	}

	protected List<String> getFieldOrder() {
        return Arrays.asList("name", "unique_id", "manufacturer", "model", "supportsExposureAuto", "supportsExposureManual", "supportsFocusAuto", "supportsFocusManual", "formats", "formats_length", "_internal");
	}
	
    public static class ByReference extends CaptureDevice implements Structure.ByReference {
        
    };
    
    public static class ByValue extends CaptureDevice implements Structure.ByValue {
        
    };
	
	public String getName() {
	    return name.getString(0, "UTF8");
	}
	
    public String getUniqueId() {
        return unique_id.getString(0, "UTF8");
    }

    public String getManufacturer() {
        return manufacturer.getString(0, "UTF8");
    }

    public String getModel() {
        return model.getString(0, "UTF8");
    }
    
    public List<CaptureFormat> getFormats() {
        CaptureFormat[] formats = (CaptureFormat[]) this.formats.toArray(formats_length);
        return Arrays.asList(formats);
    }
    
    @Override
    public String toString() {
        return String.format("name %s, uniqueId %s, manufacturer %s, model %s, formats %s", getName(), getUniqueId(), getManufacturer(), getModel(), getFormats());
    }
}

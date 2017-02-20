package org.openpnp.capture;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import java.util.Arrays;
import java.util.List;
public class CaptureDevice extends Structure {
	public Pointer name;
	public Pointer unique_id;
	public Pointer manufacturer;
	public Pointer model;
	public Pointer _internal;
	
	public CaptureDevice() {
	    super();
	}
	
	public CaptureDevice(Pointer peer) {
	    super(peer);
	}

	protected List<String> getFieldOrder() {
		return Arrays.asList("name", "unique_id", "manufacturer", "model", "_internal");
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
    
    @Override
    public String toString() {
        return String.format("%s %s %s %s", getName(), getUniqueId(), getManufacturer(), getModel());
    }
}

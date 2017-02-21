#ifndef openpnp_capture_h
#define openpnp_capture_h

typedef enum _capture_status {
    CAPTURE_OK = 0,
    CAPTURE_ERROR = -1
} capture_status;

/**
 * An opaque type that represents a context for callers to work within. The
 * caller is not expected to know anything about this value.
 */
typedef void* capture_context;

/**
 * A single capture device attached to the system. Contains information used
 * to identify the device and can be used to call into other library functions.
 */
typedef struct _capture_device {
    const char* name;
    const char* unique_id;
    const char* manufacturer;
    const char* model;
    void* _internal;
} capture_device;

capture_status create_context(capture_context** context);

capture_status release_context(capture_context* context);

capture_status list_devices(capture_context* context, capture_device** devices, unsigned int* devices_length);

#endif

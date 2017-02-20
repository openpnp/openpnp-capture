#ifndef openpnp_capture_h
#define openpnp_capture_h

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

/**
 * Create a new capture context. This is the entry point into the API and is
 * the first thing you should call to interact with it. Store the return value
 * and use it for all future calls.
 */
capture_context create_context();

/**
 * Release a capture_context when you are finished with it. This frees all
 * resources associated with the context, incuding any devices that were
 * listed or opened.
 */
void release_context(capture_context context);

/**
 * Obtain a null terminated list of capture_device pointers. Each pointer
 * represents a capture device attached to the system. Each time this function
 * is called any previous lists returned by it are released and should no
 * longer be used.
 */
capture_device** list_devices(capture_context context);

#endif

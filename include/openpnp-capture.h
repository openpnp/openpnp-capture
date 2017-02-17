#ifndef openpnp_capture_h
#define openpnp_capture_h

typedef void* capture_context;

typedef struct _capture_device {
    const char* name;
    const char* unique_id;
    const char* manufacturer;
    const char* model;
    void* _internal;
} capture_device;

capture_context create_context();

void release_context(capture_context context);

capture_device** list_devices(capture_context context);

#endif

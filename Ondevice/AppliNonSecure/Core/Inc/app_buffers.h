#ifndef APP_BUFFERS_H
#define APP_BUFFERS_H
#include <stdint.h>
#include <stddef.h>
/* Provisional capacities, independent of camera/model configuration. */
#define APP_CAMERA_FRAME_BYTES (320U * 240U * 2U)
#define APP_INFERENCE_INPUT_BYTES (320U * 240U * 3U)
#define APP_INFERENCE_OUTPUT_BYTES (64U * 1024U)
#define APP_INFERENCE_WORK_BYTES (4U * 1024U * 1024U)
#define APP_BUFFER_ALIGNMENT 64U
typedef enum {
  APP_BUFFER_CAMERA_0, APP_BUFFER_CAMERA_1, APP_BUFFER_INPUT,
  APP_BUFFER_OUTPUT, APP_BUFFER_WORK, APP_BUFFER_COUNT
} AppBufferId;
typedef struct { void *data; size_t size; } AppBuffer;
/* Startup only, after self-test and before camera/NPU use.
 * Get returns NULL before successful initialization. Initial contents are
 * undefined; this API provides storage, not ownership synchronization. */
int AppBuffers_Init(void);
const AppBuffer *AppBuffers_Get(AppBufferId id);
extern volatile uint32_t app_buffers_ready;
#endif

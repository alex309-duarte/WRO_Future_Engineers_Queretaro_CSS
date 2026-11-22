// Stand-in for the HailoRT C header, used only by the digital twin build.
// It declares the few HailoRT types that the repository's camera pipeline
// (toolbox, utils, hailo_infer.hpp, object_detection.cpp) names, so those
// files compile unchanged on a PC without a Hailo accelerator. The "model"
// behind them is sim_hailo.cpp, a colour detector that fills the same NMS
// output buffer the YOLOv8n network fills on the Hailo-8.
#pragma once

#include <cstddef>
#include <cstdint>

typedef float float32_t;

typedef enum {
    HAILO_SUCCESS = 0,
    HAILO_UNINITIALIZED,
    HAILO_INVALID_ARGUMENT,
    HAILO_OUT_OF_HOST_MEMORY,
    HAILO_TIMEOUT,
    HAILO_INTERNAL_FAILURE,
    HAILO_INVALID_OPERATION,
    HAILO_NOT_FOUND,
    HAILO_STREAM_ABORT,
} hailo_status;

typedef enum {
    HAILO_FORMAT_TYPE_AUTO = 0,
    HAILO_FORMAT_TYPE_UINT8,
    HAILO_FORMAT_TYPE_UINT16,
    HAILO_FORMAT_TYPE_FLOAT32,
} hailo_format_type_t;

typedef enum {
    HAILO_FORMAT_ORDER_AUTO = 0,
    HAILO_FORMAT_ORDER_NHWC,
    HAILO_FORMAT_ORDER_HAILO_NMS_BY_CLASS,
} hailo_format_order_t;

typedef struct {
    hailo_format_type_t type;
    hailo_format_order_t order;
    uint32_t flags;
} hailo_format_t;

typedef struct {
    uint32_t height;
    uint32_t width;
    uint32_t features;
} hailo_3d_image_shape_t;

typedef struct {
    uint32_t number_of_classes;
    uint32_t max_bboxes_per_class;
} hailo_nms_shape_t;

// Same field order as HailoRT: y_min, x_min, y_max, x_max, score (normalised).
typedef struct {
    float32_t y_min;
    float32_t x_min;
    float32_t y_max;
    float32_t x_max;
    float32_t score;
} hailo_bbox_float32_t;

#define HAILO_MAX_STREAM_NAME_SIZE 128

typedef struct {
    char name[HAILO_MAX_STREAM_NAME_SIZE];
    hailo_format_t format;
    union {
        hailo_3d_image_shape_t shape;
        hailo_nms_shape_t nms_shape;
    };
} hailo_vstream_info_t;

typedef struct {
    uint32_t device_count;
} hailo_vdevice_params_t;

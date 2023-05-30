#include <stdint.h>

enum data_type {
    CAMERA_HEADER = 0,
    OPTITRACK_HEADER = 1,
    TIMESTAMP_HEADER = 2
};

struct __attribute__((packed)) camera_header {
    uint8_t camera_id;
    uint32_t num_events;
};

struct __attribute__((packed)) camera_event {
    uint32_t t : 32;
    uint16_t x : 12;
    uint16_t y : 11;
    uint8_t p : 1;
};

struct __attribute__((packed)) optitrack_header {
    uint32_t t;
    uint8_t object_id;
    float pos_x;
    float pos_y;
    float pos_z;
    float q_x;
    float q_y;
    float q_z;
    float q_w;
};

struct __attribute__((packed)) timestamp_header {
    uint32_t t;
};
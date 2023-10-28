#include <stdint.h>

#pragma once

enum optitrack_ids {
    CAMERA0 = 0,
    CAMERA1 = 1,
    CAMERA2 = 2,
    CAMERA3 = 3,
    CAMERA4 = 4,
    CAMERA5 = 5,
    CAMERA6 = 6,
    CAMERA7 = 7,
    CAMERA8 = 8,
    CAMERA9 = 9,
    RECTANGLE0 = 11,
    SQUARE0 = 12,
    CIRCLE0 = 13,
    TRIANGLE0 = 14,
    PLIER0 = 15,
    HAMMER0 = 16,
    TEST_POINT = 128,
    CHECKERBOARD = 129
};

enum data_type {
    CAMERA_HEADER = 0,
    OPTITRACK_HEADER = 1,
    TIMESTAMP_HEADER = 2
};

struct __attribute__((packed)) camera_header {
    uint32_t pc_t;
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
    uint32_t pc_t;
    uint32_t t;
    uint8_t object_id;
    int16_t params;
    float error;
    float pos_x;
    float pos_y;
    float pos_z;
    float q_x;
    float q_y;
    float q_z;
    float q_w;
};

struct __attribute__((packed)) timestamp_header {
    uint32_t pc_t;
};
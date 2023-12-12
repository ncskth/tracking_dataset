#pragma once

#include "flow_struct.h"

extern uint32_t num_events;

void init_camera(struct flow_struct &data);

extern uint8_t camera_frame[1280*720];
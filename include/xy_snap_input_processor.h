#pragma once

#include <zmk/event_manager.h>

// 前方宣言
struct zmk_input_event;

int xy_snap_input_processor(struct zmk_input_event *event); 
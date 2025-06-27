#ifndef ZMK_XY_SNAP_INPUT_PROCESSOR_H
#define ZMK_XY_SNAP_INPUT_PROCESSOR_H

#include <zmk/event_manager.h>

// ZMK input-event構造体の定義
struct zmk_input_event {
    void *data;
    // 他のメンバーは必要に応じて追加
};

int xy_snap_input_processor(struct zmk_input_event *event);

#endif // ZMK_XY_SNAP_INPUT_PROCESSOR_H 
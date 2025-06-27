#ifndef ZMK_XY_SNAP_INPUT_PROCESSOR_H
#define ZMK_XY_SNAP_INPUT_PROCESSOR_H

#include <zephyr/kernel.h>
#include <zmk/input/processor.h>

// ZMK input-event構造体の定義
struct zmk_input_event {
    void *data;
    // 他のメンバーは必要に応じて追加
};

// ZMK input-processor APIに準拠した関数宣言
int zmk_xy_snap_input_processor_process(struct zmk_input_processor *processor,
                                       struct zmk_input_event *event);

#endif // ZMK_XY_SNAP_INPUT_PROCESSOR_H 
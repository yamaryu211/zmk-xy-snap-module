#ifndef ZMK_XY_SNAP_INPUT_PROCESSOR_H
#define ZMK_XY_SNAP_INPUT_PROCESSOR_H

#include <zephyr/kernel.h>

// ZMK input-processor構造体の前方宣言
struct zmk_input_processor;

// ZMK input-processor API構造体の前方宣言
struct zmk_input_processor_api;

// ZMK input-event構造体の定義
struct zmk_input_event {
    void *data;
    int type;
    // 他のメンバーは必要に応じて追加
};

// ZMK pointer-event構造体の定義
struct zmk_pointer_event {
    int x;
    int y;
    int type;
    // 他のメンバーは必要に応じて追加
};

// ZMK input-processor APIに準拠した関数宣言
int zmk_xy_snap_input_processor_process(struct zmk_input_processor *processor,
                                       struct zmk_input_event *event);

// イベントタイプの定義
#define ZMK_INPUT_EVENT_POINTER 1

#endif // ZMK_XY_SNAP_INPUT_PROCESSOR_H

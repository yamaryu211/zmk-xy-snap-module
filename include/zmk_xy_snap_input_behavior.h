#ifndef ZMK_XY_SNAP_INPUT_BEHAVIOR_H
#define ZMK_XY_SNAP_INPUT_BEHAVIOR_H

#include <zephyr/kernel.h>
#include <zmk/input/input_behavior.h>
#include <zmk/input/input_listener.h>

// ZMK input-event構造体の定義
struct zmk_input_event {
    void *data;
    // 他のメンバーは必要に応じて追加
};

// input-behavior API関数宣言
int zmk_xy_snap_input_behavior_binding_pressed(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event);

int zmk_xy_snap_input_behavior_binding_released(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event);

// input-listener API関数宣言
int zmk_xy_snap_input_listener_callback(struct zmk_input_listener *listener,
                                       struct zmk_input_event *event);

#endif // ZMK_XY_SNAP_INPUT_BEHAVIOR_H 
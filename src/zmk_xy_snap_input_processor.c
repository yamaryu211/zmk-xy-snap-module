#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdlib.h>
#include "zmk_xy_snap_input_processor.h"

// Kconfigパラメータ取得（デフォルト値を設定）
#ifndef CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS
#define CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS 300
#endif

#ifndef CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD
#define CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD 50
#endif

#ifndef CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH
#define CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH 0
#endif

#ifndef CONFIG_ZMK_XY_SNAP_INITIAL_THRESHOLD
#define CONFIG_ZMK_XY_SNAP_INITIAL_THRESHOLD 10
#endif

#define XY_SNAP_IDLE_TIMEOUT_MS CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS
#define XY_SNAP_SWITCH_THRESHOLD CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD
#define XY_SNAP_ALLOW_AXIS_SWITCH CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH
#define XY_SNAP_INITIAL_THRESHOLD CONFIG_ZMK_XY_SNAP_INITIAL_THRESHOLD

// 状態管理用構造体
struct xy_snap_state {
    bool axis_locked;
    bool allow_axis_switch;
    int locked_axis; // 0:未固定, 1:X, 2:Y
    int64_t last_move_time;
    int accumulated_x;
    int accumulated_y;
    bool initial_direction_determined;
};

static struct xy_snap_state snap_state = {
    .axis_locked = false,
    .allow_axis_switch = XY_SNAP_ALLOW_AXIS_SWITCH,
    .locked_axis = 0,
    .last_move_time = 0,
    .accumulated_x = 0,
    .accumulated_y = 0,
    .initial_direction_determined = false,
};

// pointerイベントか判定
static bool is_pointer_event(struct zmk_input_event *event) {
    return event && event->type == ZMK_INPUT_EVENT_POINTER;
}

int zmk_xy_snap_input_processor_process(struct zmk_input_processor *processor,
                                       struct zmk_input_event *event) {
    if (!is_pointer_event(event)) {
        return 0; // pointerイベント以外は処理しない
    }

    struct zmk_pointer_event *pointer_event = (struct zmk_pointer_event *)event;
    int x = pointer_event->x;
    int y = pointer_event->y;
    int64_t now = k_uptime_get();

    // 停止判定（両軸0）
    if (x == 0 && y == 0) {
        if (snap_state.axis_locked && (now - snap_state.last_move_time > XY_SNAP_IDLE_TIMEOUT_MS)) {
            // リセット
            snap_state.axis_locked = false;
            snap_state.locked_axis = 0;
            snap_state.accumulated_x = 0;
            snap_state.accumulated_y = 0;
            snap_state.initial_direction_determined = false;
        }
        return 0;
    }

    // 軸未固定→初期方向の決定
    if (!snap_state.axis_locked) {
        snap_state.accumulated_x += x;
        snap_state.accumulated_y += y;
        
        // 初期方向の決定（累積値で判定）
        if (!snap_state.initial_direction_determined) {
            if (abs(snap_state.accumulated_x) >= XY_SNAP_INITIAL_THRESHOLD || 
                abs(snap_state.accumulated_y) >= XY_SNAP_INITIAL_THRESHOLD) {
                
                if (abs(snap_state.accumulated_x) >= abs(snap_state.accumulated_y)) {
                    snap_state.locked_axis = 1; // X軸固定
                } else {
                    snap_state.locked_axis = 2; // Y軸固定
                }
                snap_state.initial_direction_determined = true;
                snap_state.axis_locked = true;
                snap_state.last_move_time = now;
            }
        }
    } else {
        // 軸固定中
        if (snap_state.locked_axis == 1) { // X軸固定
            if (snap_state.allow_axis_switch && abs(y) > XY_SNAP_SWITCH_THRESHOLD) {
                // 軸切り替え（iPhone風の挙動では通常無効）
                snap_state.locked_axis = 2;
                snap_state.accumulated_x = 0;
                snap_state.accumulated_y = y;
            } else {
                pointer_event->y = 0; // Y方向は無視
            }
        } else if (snap_state.locked_axis == 2) { // Y軸固定
            if (snap_state.allow_axis_switch && abs(x) > XY_SNAP_SWITCH_THRESHOLD) {
                // 軸切り替え（iPhone風の挙動では通常無効）
                snap_state.locked_axis = 1;
                snap_state.accumulated_x = x;
                snap_state.accumulated_y = 0;
            } else {
                pointer_event->x = 0; // X方向は無視
            }
        }
        snap_state.last_move_time = now;
    }
    return 0;
}

// 完全なAPI構造体の定義
static const struct zmk_input_processor_api xy_snap_api = {
    .process = (int (*)(struct zmk_input_processor *, void *))zmk_xy_snap_input_processor_process,
};

// デバイス定義 - 正しいノードラベルを使用
#ifdef DT_N_NODELABEL_xy_snap

DEVICE_DT_DEFINE(DT_NODELABEL(xy_snap), NULL, NULL, NULL, NULL,
                POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
                &xy_snap_api);

#endif

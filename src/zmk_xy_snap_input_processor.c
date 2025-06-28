#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdlib.h>
#include "zmk_xy_snap_input_processor.h"

// Kconfigパラメータ取得（デフォルト値を設定）
#ifndef CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS
#define CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS 200
#endif

#ifndef CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD
#define CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD 20
#endif

#ifndef CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH
#define CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH 1
#endif

#ifndef CONFIG_ZMK_XY_SNAP_SWITCH_RATIO
#define CONFIG_ZMK_XY_SNAP_SWITCH_RATIO 30
#endif

#ifndef CONFIG_ZMK_XY_SNAP_CONTINUE_TIMEOUT_MS
#define CONFIG_ZMK_XY_SNAP_CONTINUE_TIMEOUT_MS 1000
#endif

#define XY_SNAP_IDLE_TIMEOUT_MS CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS
#define XY_SNAP_SWITCH_THRESHOLD CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD
#define XY_SNAP_ALLOW_AXIS_SWITCH CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH
#define XY_SNAP_SWITCH_RATIO CONFIG_ZMK_XY_SNAP_SWITCH_RATIO
#define XY_SNAP_CONTINUE_TIMEOUT_MS CONFIG_ZMK_XY_SNAP_CONTINUE_TIMEOUT_MS

// 動作モードの定義
#ifdef CONFIG_ZMK_XY_SNAP_MODE_KEYBALL
#define XY_SNAP_MODE_KEYBALL 1
#else
#define XY_SNAP_MODE_KEYBALL 0
#endif

// 状態管理用構造体
struct xy_snap_state {
    bool axis_locked;
    bool allow_axis_switch;
    int locked_axis; // 0:未固定, 1:縦軸固定, 2:横軸固定
    int64_t last_move_time;
    int64_t last_stop_time;
    bool in_continue_period;
};

static struct xy_snap_state snap_state = {
    .axis_locked = false,
    .allow_axis_switch = XY_SNAP_ALLOW_AXIS_SWITCH,
    .locked_axis = 0,
    .last_move_time = 0,
    .last_stop_time = 0,
    .in_continue_period = false,
};

// pointerイベントか判定
static bool is_pointer_event(struct zmk_input_event *event) {
    return event && event->type == ZMK_INPUT_EVENT_POINTER;
}

// 絶対値計算
static int abs_val(int val) {
    return val < 0 ? -val : val;
}

// 軸の決定（Traditional Mode）
static int determine_axis_traditional(int x, int y) {
    if (abs_val(x) >= abs_val(y)) {
        return 1; // X軸固定
    } else {
        return 2; // Y軸固定
    }
}

// 軸の決定（Keyball Mode）
static int determine_axis_keyball(int x, int y) {
    int abs_x = abs_val(x);
    int abs_y = abs_val(y);
    
    // 横方向成分が縦方向成分の一定割合以上の場合、横軸に固定
    if (abs_x * 100 >= abs_y * XY_SNAP_SWITCH_RATIO) {
        return 2; // 横軸固定
    } else {
        return 1; // 縦軸固定（デフォルト）
    }
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
        if (snap_state.axis_locked) {
            snap_state.last_stop_time = now;
            snap_state.in_continue_period = true;
        }
        return 0;
    }

    if (XY_SNAP_MODE_KEYBALL) {
        // Keyball Mode の処理
        // 継続期間の判定
        if (snap_state.in_continue_period) {
            if (now - snap_state.last_stop_time > XY_SNAP_CONTINUE_TIMEOUT_MS) {
                snap_state.in_continue_period = false;
                snap_state.axis_locked = false;
                snap_state.locked_axis = 0;
            }
        }

        // 軸未固定または継続期間外の場合
        if (!snap_state.axis_locked || !snap_state.in_continue_period) {
            snap_state.locked_axis = determine_axis_keyball(x, y);
            snap_state.axis_locked = true;
            snap_state.last_move_time = now;
            snap_state.in_continue_period = false;
        }
    } else {
        // Traditional Mode の処理
        // 軸未固定→動き出し方向で軸固定
        if (!snap_state.axis_locked) {
            snap_state.locked_axis = determine_axis_traditional(x, y);
            snap_state.axis_locked = true;
            snap_state.last_move_time = now;
        } else {
            // 軸固定中
            if (snap_state.locked_axis == 1) { // X軸固定
                // Y方向の大きな動きで切り替え
                if (snap_state.allow_axis_switch && abs_val(y) > XY_SNAP_SWITCH_THRESHOLD) {
                    snap_state.locked_axis = 2;
                } else {
                    pointer_event->y = 0; // Y方向は無視
                }
            } else if (snap_state.locked_axis == 2) { // Y軸固定
                if (snap_state.allow_axis_switch && abs_val(x) > XY_SNAP_SWITCH_THRESHOLD) {
                    snap_state.locked_axis = 1;
                } else {
                    pointer_event->x = 0; // X方向は無視
                }
            }
            snap_state.last_move_time = now;
        }
    }

    // 軸固定中の処理（共通）
    if (snap_state.locked_axis == 1) { // 縦軸固定
        pointer_event->x = 0; // 横方向は無視
    } else if (snap_state.locked_axis == 2) { // 横軸固定
        pointer_event->y = 0; // 縦方向は無視
    }

    snap_state.last_move_time = now;
    return 0;
}

// 完全なAPI構造体の定義
static const struct zmk_input_processor_api xy_snap_api = {
    .process = (int (*)(struct zmk_input_processor *, void *))zmk_xy_snap_input_processor_process,
};

// デバイス定義 - 条件付きコンパイルで安全に定義
#ifdef DT_N_NODELABEL_xy_snap
DEVICE_DT_DEFINE(DT_NODELABEL(xy_snap), NULL, NULL, NULL, NULL,
                POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
                &xy_snap_api);
#endif

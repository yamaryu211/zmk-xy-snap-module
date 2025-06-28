#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zmk/input/processor.h>
#include <zmk/events.h>
#include <zmk/event_manager.h>
#include <zmk/input/input.h>
#include <zmk/input/input_listener.h>
#include <zmk/input/input_processor.h>
#include "zmk_xy_snap_input_processor.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

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

#define XY_SNAP_IDLE_TIMEOUT_MS CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS
#define XY_SNAP_SWITCH_THRESHOLD CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD
#define XY_SNAP_ALLOW_AXIS_SWITCH CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH

// 状態管理用構造体
struct xy_snap_state {
    bool axis_locked;
    bool allow_axis_switch;
    int locked_axis; // 0:未固定, 1:X, 2:Y
    int64_t last_move_time;
};

static struct xy_snap_state snap_state = {
    .axis_locked = false,
    .allow_axis_switch = XY_SNAP_ALLOW_AXIS_SWITCH,
    .locked_axis = 0,
    .last_move_time = 0,
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
            snap_state.axis_locked = false;
            snap_state.locked_axis = 0;
            LOG_DBG("Axis lock released due to timeout");
        }
        return 0;
    }

    // 軸未固定→動き出し方向で軸固定
    if (!snap_state.axis_locked) {
        if (abs(x) >= abs(y)) {
            snap_state.locked_axis = 1; // X軸固定
            LOG_DBG("Axis locked to X");
        } else {
            snap_state.locked_axis = 2; // Y軸固定
            LOG_DBG("Axis locked to Y");
        }
        snap_state.axis_locked = true;
        snap_state.last_move_time = now;
    } else {
        // 軸固定中
        if (snap_state.locked_axis == 1) { // X軸固定
            // Y方向の大きな動きで切り替え
            if (snap_state.allow_axis_switch && abs(y) > XY_SNAP_SWITCH_THRESHOLD) {
                snap_state.locked_axis = 2;
                LOG_DBG("Axis switched from X to Y");
            } else {
                pointer_event->y = 0; // Y方向は無視
            }
        } else if (snap_state.locked_axis == 2) { // Y軸固定
            if (snap_state.allow_axis_switch && abs(x) > XY_SNAP_SWITCH_THRESHOLD) {
                snap_state.locked_axis = 1;
                LOG_DBG("Axis switched from Y to X");
            } else {
                pointer_event->x = 0; // X方向は無視
            }
        }
        snap_state.last_move_time = now;
    }
    return 0;
}

// ZMK input-processor APIの登録
ZMK_INPUT_PROCESSOR_API_DEFINE(xy_snap, zmk_xy_snap_input_processor_process);

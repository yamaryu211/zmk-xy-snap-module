#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zmk/input/input_listener.h>
#include <zmk/input/input_behavior.h>
#include <zmk/events.h>
#include <zmk/event_manager.h>
#include <zmk/input/input.h>
#include "zmk_xy_snap_input_behavior.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Kconfigパラメータ取得
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
    int pending_x;
    int pending_y;
};

static struct xy_snap_state snap_state = {
    .axis_locked = false,
    .allow_axis_switch = XY_SNAP_ALLOW_AXIS_SWITCH,
    .locked_axis = 0,
    .last_move_time = 0,
    .pending_x = 0,
    .pending_y = 0,
};

// input-behavior APIの実装
int zmk_xy_snap_input_behavior_binding_pressed(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    // 軸ロックを開始
    snap_state.axis_locked = true;
    snap_state.last_move_time = k_uptime_get();
    
    // 最初の動きで軸を決定
    if (abs(snap_state.pending_x) >= abs(snap_state.pending_y)) {
        snap_state.locked_axis = 1; // X軸固定
        LOG_DBG("Axis locked to X");
    } else {
        snap_state.locked_axis = 2; // Y軸固定
        LOG_DBG("Axis locked to Y");
    }
    
    return ZMK_BEHAVIOR_OPAQUE;
}

int zmk_xy_snap_input_behavior_binding_released(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event) {
    // 軸ロックを解除
    snap_state.axis_locked = false;
    snap_state.locked_axis = 0;
    snap_state.pending_x = 0;
    snap_state.pending_y = 0;
    LOG_DBG("Axis lock released");
    
    return ZMK_BEHAVIOR_OPAQUE;
}

// input-listener APIの実装
int zmk_xy_snap_input_listener_callback(struct zmk_input_listener *listener,
                                       struct zmk_input_event *event) {
    if (!event || event->type != ZMK_INPUT_EVENT_POINTER) {
        return 0;
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

    // 軸固定中の処理
    if (snap_state.axis_locked) {
        if (snap_state.locked_axis == 1) { // X軸固定
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

// input-behavior APIの登録
ZMK_BEHAVIOR_DEFINITION(xy_snap_input_behavior, 
                       zmk_xy_snap_input_behavior_binding_pressed,
                       zmk_xy_snap_input_behavior_binding_released, 
                       NULL, NULL);

// input-listener APIの登録
ZMK_INPUT_LISTENER_API_DEFINE(xy_snap_input_listener, zmk_xy_snap_input_listener_callback); 
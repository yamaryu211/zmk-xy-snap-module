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

// input-behavior APIの実装
int zmk_xy_snap_input_behavior_binding_pressed(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    // 軸ロックを開始
    snap_state.axis_locked = true;
    snap_state.last_move_time = k_uptime_get();
    
    // 累積値で初期方向を決定
    if (abs(snap_state.accumulated_x) >= abs(snap_state.accumulated_y)) {
        snap_state.locked_axis = 1; // X軸固定
        LOG_DBG("Axis locked to X (accumulated: x=%d, y=%d)", snap_state.accumulated_x, snap_state.accumulated_y);
    } else {
        snap_state.locked_axis = 2; // Y軸固定
        LOG_DBG("Axis locked to Y (accumulated: x=%d, y=%d)", snap_state.accumulated_x, snap_state.accumulated_y);
    }
    
    return ZMK_BEHAVIOR_OPAQUE;
}

int zmk_xy_snap_input_behavior_binding_released(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event) {
    // 軸ロックを解除
    snap_state.axis_locked = false;
    snap_state.locked_axis = 0;
    snap_state.accumulated_x = 0;
    snap_state.accumulated_y = 0;
    snap_state.initial_direction_determined = false;
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
            snap_state.accumulated_x = 0;
            snap_state.accumulated_y = 0;
            snap_state.initial_direction_determined = false;
            LOG_DBG("Axis lock released due to timeout");
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
                LOG_DBG("Initial direction determined: %s (accumulated: x=%d, y=%d)", 
                       snap_state.locked_axis == 1 ? "X" : "Y", 
                       snap_state.accumulated_x, snap_state.accumulated_y);
            }
        }
    } else {
        // 軸固定中の処理
        if (snap_state.locked_axis == 1) { // X軸固定
            if (snap_state.allow_axis_switch && abs(y) > XY_SNAP_SWITCH_THRESHOLD) {
                snap_state.locked_axis = 2;
                snap_state.accumulated_x = 0;
                snap_state.accumulated_y = y;
                LOG_DBG("Axis switched from X to Y");
            } else {
                pointer_event->y = 0; // Y方向は無視
            }
        } else if (snap_state.locked_axis == 2) { // Y軸固定
            if (snap_state.allow_axis_switch && abs(x) > XY_SNAP_SWITCH_THRESHOLD) {
                snap_state.locked_axis = 1;
                snap_state.accumulated_x = x;
                snap_state.accumulated_y = 0;
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
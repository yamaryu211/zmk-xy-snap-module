#include <zmk/event_manager.h>
#include <zmk/behavior.h>
#include <kernel.h>
#include <device.h>
#include "xy_snap_input_processor.h"

// Kconfigパラメータ取得
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

// pointerイベントの型定義例（ZMK/pmw3610-driver準拠）
struct zmk_hid_mouse_report {
    int8_t x;
    int8_t y;
    // ...他フィールド省略
};

// pointerイベントか判定
static bool is_pointer_event(struct zmk_input_event *event) {
    // pmw3610-driverやZMKのpointerイベント判定（仮実装）
    // 実際はevent->typeやevent->report_type等で判定する場合も
    return event && event->data && ((struct zmk_hid_mouse_report *)event->data);
}

int xy_snap_input_processor(struct zmk_input_event *event) {
    if (!is_pointer_event(event)) {
        return 0; // pointerイベント以外は処理しない
    }
    struct zmk_hid_mouse_report *report = (struct zmk_hid_mouse_report *)event->data;
    int x = report->x;
    int y = report->y;
    int64_t now = k_uptime_get();

    // 停止判定（両軸0）
    if (x == 0 && y == 0) {
        if (snap_state.axis_locked && (now - snap_state.last_move_time > XY_SNAP_IDLE_TIMEOUT_MS)) {
            snap_state.axis_locked = false;
            snap_state.locked_axis = 0;
        }
        return 0;
    }

    // 軸未固定→動き出し方向で軸固定
    if (!snap_state.axis_locked) {
        if (abs(x) >= abs(y)) {
            snap_state.locked_axis = 1; // X軸固定
        } else {
            snap_state.locked_axis = 2; // Y軸固定
        }
        snap_state.axis_locked = true;
        snap_state.last_move_time = now;
    } else {
        // 軸固定中
        if (snap_state.locked_axis == 1) { // X軸固定
            // Y方向の大きな動きで切り替え
            if (snap_state.allow_axis_switch && abs(y) > XY_SNAP_SWITCH_THRESHOLD) {
                snap_state.locked_axis = 2;
            } else {
                report->y = 0; // Y方向は無視
            }
        } else if (snap_state.locked_axis == 2) { // Y軸固定
            if (snap_state.allow_axis_switch && abs(x) > XY_SNAP_SWITCH_THRESHOLD) {
                snap_state.locked_axis = 1;
            } else {
                report->x = 0; // X方向は無視
            }
        }
        snap_state.last_move_time = now;
    }
    return 0;
}

ZMK_INPUT_PROCESSOR(xy_snap_input_processor); 
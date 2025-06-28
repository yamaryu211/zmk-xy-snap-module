#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdlib.h>
#include "zmk_xy_snap_input_processor.h"

// 数学ライブラリの使用を選択
#ifdef CONFIG_ZMK_XY_SNAP_USE_MATH_LIB
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#endif

// Kconfigパラメータ取得（デフォルト値を設定）
#ifndef CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS
#define CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS 200
#endif

#ifndef CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD
#define CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD 20
#endif

#ifndef CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH
#define CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH 0
#endif

#ifndef CONFIG_ZMK_XY_SNAP_MIN_MOVE_THRESHOLD
#define CONFIG_ZMK_XY_SNAP_MIN_MOVE_THRESHOLD 5
#endif

#define XY_SNAP_IDLE_TIMEOUT_MS CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS
#define XY_SNAP_SWITCH_THRESHOLD CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD
#define XY_SNAP_ALLOW_AXIS_SWITCH CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH
#define XY_SNAP_MIN_MOVE_THRESHOLD CONFIG_ZMK_XY_SNAP_MIN_MOVE_THRESHOLD

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

#ifdef CONFIG_ZMK_XY_SNAP_USE_MATH_LIB
// 数学ライブラリを使用した高精度な軸選択
static int determine_axis_from_vector(int x, int y) {
    // 最小移動量のチェック
    double magnitude = sqrt(x*x + y*y);
    if (magnitude < XY_SNAP_MIN_MOVE_THRESHOLD) {
        return 0; // 移動量が小さすぎる場合は軸選択しない
    }
    
    // 移動ベクトルの角度を計算（ラジアン）
    double angle = atan2(y, x);
    
    // 角度を度数法に変換
    double degrees = angle * 180.0 / M_PI;
    
    // 角度を正規化（0〜360°）
    if (degrees < 0) {
        degrees += 360;
    }
    
    // 軸選択の判定
    // 水平方向: 0°〜45°、315°〜360°
    // 垂直方向: 45°〜315°
    if ((degrees >= 0 && degrees <= 45) || (degrees >= 315 && degrees <= 360)) {
        return 1; // X軸固定
    } else {
        return 2; // Y軸固定
    }
}
#else
// 簡易的な平方根計算（整数用）
static int sqrt_int(int x) {
    if (x <= 0) return 0;
    if (x == 1) return 1;
    
    int result = 1;
    int i = 1;
    while (i * i <= x) {
        result = i;
        i++;
    }
    return result;
}

// 軽量な軸選択（整数演算のみ）
static int determine_axis_from_vector(int x, int y) {
    // 最小移動量のチェック（整数平方根を使用）
    int magnitude_squared = x*x + y*y;
    int magnitude = sqrt_int(magnitude_squared);
    if (magnitude < XY_SNAP_MIN_MOVE_THRESHOLD) {
        return 0; // 移動量が小さすぎる場合は軸選択しない
    }
    
    // 移動ベクトルの角度を計算（簡易版）
    // atan2の代わりに、xとyの比率を使用
    int abs_x = abs(x);
    int abs_y = abs(y);
    
    // 角度の判定（45度 = 1:1の比率）
    // 水平方向: |x| >= |y|
    // 垂直方向: |y| > |x|
    if (abs_x >= abs_y) {
        return 1; // X軸固定
    } else {
        return 2; // Y軸固定
    }
}
#endif

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
        }
        return 0;
    }

    // 軸未固定→移動ベクトルで軸固定
    if (!snap_state.axis_locked) {
        int selected_axis = determine_axis_from_vector(x, y);
        if (selected_axis > 0) {
            snap_state.locked_axis = selected_axis;
            snap_state.axis_locked = true;
            snap_state.last_move_time = now;
        }
    } else {
        // 軸固定中
        if (snap_state.locked_axis == 1) { // X軸固定
            // Y方向の大きな動きで切り替え
            if (snap_state.allow_axis_switch && abs(y) > XY_SNAP_SWITCH_THRESHOLD) {
                snap_state.locked_axis = 2;
            } else {
                pointer_event->y = 0; // Y方向は無視
            }
        } else if (snap_state.locked_axis == 2) { // Y軸固定
            if (snap_state.allow_axis_switch && abs(x) > XY_SNAP_SWITCH_THRESHOLD) {
                snap_state.locked_axis = 1;
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

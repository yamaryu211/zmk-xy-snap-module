/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// INPUT_REL_Xとinput_REL_Yの定義
#define INPUT_REL_X 0x00
#define INPUT_REL_Y 0x01

// 設定構造体の定義
struct xy_snap_config {
    int32_t idle_timeout_ms;
    int32_t switch_threshold;
    int32_t initial_threshold;
    bool allow_axis_switch;
    bool track_remainders;
};

// データ構造体の定義
struct xy_snap_data {
    int32_t accumulated_x;
    int32_t accumulated_y;
    bool axis_locked;
    bool lock_to_x;
    int64_t last_activity_time;
    // track_remainders用
    int32_t remainder_x;
    int32_t remainder_y;
};

static void xy_snap_reset_state(struct xy_snap_data *data) {
    data->accumulated_x = 0;
    data->accumulated_y = 0;
    data->axis_locked = false;
    data->lock_to_x = false;
    data->last_activity_time = 0;
    data->remainder_x = 0;
    data->remainder_y = 0;
}

static void xy_snap_callback(const struct device *dev, struct input_event *evt) {
    const struct xy_snap_config *config = dev->config;
    struct xy_snap_data *data = dev->data;
    
    // 相対位置イベントのみ処理
    if (evt->type != INPUT_EV_REL) {
        return;
    }
    
    // タイムアウト処理
    int64_t now = k_uptime_get();
    if (data->last_activity_time > 0 && 
        (now - data->last_activity_time) > config->idle_timeout_ms) {
        xy_snap_reset_state(data);
    }
    data->last_activity_time = now;
    
    // 座標の累積
    if (evt->code == INPUT_REL_X) {
        data->accumulated_x += abs(evt->value);
    } else if (evt->code == INPUT_REL_Y) {
        data->accumulated_y += abs(evt->value);
    } else {
        return;
    }
    
    // 初期軸の決定
    if (!data->axis_locked) {
        int32_t threshold = config->initial_threshold;
        if (data->accumulated_x > threshold || data->accumulated_y > threshold) {
            data->axis_locked = true;
            data->lock_to_x = (data->accumulated_x > data->accumulated_y);
        }
    }
    
    // 軸切り替えの判定
    if (data->axis_locked && config->allow_axis_switch) {
        int32_t threshold = config->switch_threshold;
        if ((data->lock_to_x && data->accumulated_y > threshold) ||
            (!data->lock_to_x && data->accumulated_x > threshold)) {
            data->lock_to_x = !data->lock_to_x;
            data->accumulated_x = 0;
            data->accumulated_y = 0;
        }
    }
    
    // イベントの変更
    if (data->axis_locked) {
        if (data->lock_to_x) {
            if (evt->code == INPUT_REL_Y) {
                evt->value = 0;  // Y軸の動きを無効化
            }
        } else {
            if (evt->code == INPUT_REL_X) {
                evt->value = 0;  // X軸の動きを無効化
            }
        }
    }
}

static int xy_snap_init(const struct device *dev) {
    struct xy_snap_data *data = dev->data;
    xy_snap_reset_state(data);
    return 0;
}

#define XY_SNAP_INIT(inst)                                                     \
    static struct xy_snap_data xy_snap_data_##inst = {0};                      \
    static const struct xy_snap_config xy_snap_config_##inst = {               \
        .idle_timeout_ms = DT_INST_PROP_OR(inst, idle_timeout_ms, 300),       \
        .switch_threshold = DT_INST_PROP_OR(inst, switch_threshold, 50),      \
        .initial_threshold = DT_INST_PROP_OR(inst, initial_threshold, 10),    \
        .allow_axis_switch = DT_INST_PROP(inst, allow_axis_switch),           \
        .track_remainders = DT_INST_PROP(inst, track_remainders),             \
    };                                                                         \
    DEVICE_DT_INST_DEFINE(inst, xy_snap_init, NULL,                          \
                          &xy_snap_data_##inst, &xy_snap_config_##inst,       \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,             \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(XY_SNAP_INIT) 
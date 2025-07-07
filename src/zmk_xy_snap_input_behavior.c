/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Linux input event codes
#define INPUT_EV_REL 0x02
#define INPUT_REL_X 0x00
#define INPUT_REL_Y 0x01

struct xy_snap_config {
    int32_t idle_timeout_ms;
    int32_t switch_threshold;
    int32_t initial_threshold;
    bool allow_axis_switch;
};

struct xy_snap_data {
    int32_t accumulated_x;
    int32_t accumulated_y;
    bool axis_locked;
    bool lock_to_x;
    int64_t last_activity_time;
};

static void xy_snap_reset_state(struct xy_snap_data *data) {
    data->accumulated_x = 0;
    data->accumulated_y = 0;
    data->axis_locked = false;
    data->lock_to_x = false;
    data->last_activity_time = k_uptime_get();
}

static void xy_snap_process_event(const struct device *dev, struct input_event *event) {
    const struct xy_snap_config *config = dev->config;
    struct xy_snap_data *data = dev->data;
    
    if (event->type != INPUT_EV_REL) {
        return;
    }
    
    int64_t current_time = k_uptime_get();
    
    // タイムアウトチェック
    if (current_time - data->last_activity_time > config->idle_timeout_ms) {
        xy_snap_reset_state(data);
    }
    
    data->last_activity_time = current_time;
    
    int32_t delta_x = 0, delta_y = 0;
    
    if (event->code == INPUT_REL_X) {
        delta_x = event->value;
    } else if (event->code == INPUT_REL_Y) {
        delta_y = event->value;
    } else {
        return;
    }
    
    // 累積値を更新
    data->accumulated_x += abs(delta_x);
    data->accumulated_y += abs(delta_y);
    
    // 軸ロックの判定
    if (!data->axis_locked) {
        if (data->accumulated_x >= config->initial_threshold || 
            data->accumulated_y >= config->initial_threshold) {
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
            if (event->code == INPUT_REL_Y) {
                event->value = 0;  // Y軸の動きを無効化
            }
        } else {
            if (event->code == INPUT_REL_X) {
                event->value = 0;  // X軸の動きを無効化
            }
        }
    }
}

static int xy_snap_init(const struct device *dev) {
    struct xy_snap_data *data = dev->data;
    xy_snap_reset_state(data);
    return 0;
}

// デバイスツリーからの設定読み込み
#define XY_SNAP_INIT(inst)                                                     \
    static struct xy_snap_data xy_snap_data_##inst = {0};                      \
    static const struct xy_snap_config xy_snap_config_##inst = {               \
        .idle_timeout_ms = DT_INST_PROP_OR(inst, idle_timeout_ms, 300),       \
        .switch_threshold = DT_INST_PROP_OR(inst, switch_threshold, 50),      \
        .initial_threshold = DT_INST_PROP_OR(inst, initial_threshold, 10),    \
        .allow_axis_switch = DT_INST_PROP(inst, allow_axis_switch),           \
    };                                                                         \
    DEVICE_DT_INST_DEFINE(inst, xy_snap_init, NULL,                          \
                          &xy_snap_data_##inst, &xy_snap_config_##inst,       \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(XY_SNAP_INIT) 
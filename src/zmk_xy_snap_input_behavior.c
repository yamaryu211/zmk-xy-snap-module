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

#include <zmk/event_manager.h>
#include <zmk/events/mouse_move.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct xy_snap_config {
    uint32_t idle_timeout_ms;
    uint32_t switch_threshold;
    uint32_t initial_threshold;
    bool allow_axis_switch;
    bool track_remainders;
};

struct xy_snap_data {
    int64_t last_activity_time;
    bool axis_locked;
    bool x_axis_locked;
    bool y_axis_locked;
    int16_t accumulated_x;
    int16_t accumulated_y;
    int16_t remainder_x;
    int16_t remainder_y;
    bool initial_direction_set;
};

static void xy_snap_reset_state(struct xy_snap_data *data) {
    data->axis_locked = false;
    data->x_axis_locked = false;
    data->y_axis_locked = false;
    data->accumulated_x = 0;
    data->accumulated_y = 0;
    data->initial_direction_set = false;
    if (!data->track_remainders) {
        data->remainder_x = 0;
        data->remainder_y = 0;
    }
}

static void xy_snap_input_handler(const struct device *dev, struct input_event *evt) {
    const struct xy_snap_config *config = dev->config;
    struct xy_snap_data *data = dev->data;
    
    if (evt->type != INPUT_EV_REL) {
        // REL以外のイベントはそのまま次のprocessorに渡す
        input_report(dev, evt->type, evt->code, evt->value, evt->sync, K_FOREVER);
        return;
    }
    
    int64_t now = k_uptime_get();
    
    // タイムアウトチェック
    if (config->idle_timeout_ms > 0 && 
        (now - data->last_activity_time) > config->idle_timeout_ms) {
        xy_snap_reset_state(data);
    }
    
    data->last_activity_time = now;
    
    int16_t dx = 0, dy = 0;
    
    if (evt->code == INPUT_REL_X) {
        dx = evt->value;
    } else if (evt->code == INPUT_REL_Y) {
        dy = evt->value;
    } else {
        // X/Y以外のRELイベントはそのまま次のprocessorに渡す
        input_report_rel(dev, evt->code, evt->value, evt->sync, K_FOREVER);
        return;
    }
    
    // 余りを加算
    if (config->track_remainders) {
        dx += data->remainder_x;
        dy += data->remainder_y;
        data->remainder_x = 0;
        data->remainder_y = 0;
    }
    
    data->accumulated_x += dx;
    data->accumulated_y += dy;
    
    // 初期方向決定
    if (!data->initial_direction_set) {
        if (abs(data->accumulated_x) >= config->initial_threshold || 
            abs(data->accumulated_y) >= config->initial_threshold) {
            
            if (abs(data->accumulated_x) > abs(data->accumulated_y)) {
                data->x_axis_locked = true;
                data->y_axis_locked = false;
            } else {
                data->x_axis_locked = false;
                data->y_axis_locked = true;
            }
            data->axis_locked = true;
            data->initial_direction_set = true;
        }
    }
    
    // 軸切り替えチェック
    if (config->allow_axis_switch && data->axis_locked) {
        if ((data->x_axis_locked && abs(data->accumulated_y) > config->switch_threshold) ||
            (data->y_axis_locked && abs(data->accumulated_x) > config->switch_threshold)) {
            
            if (abs(data->accumulated_x) > abs(data->accumulated_y)) {
                data->x_axis_locked = true;
                data->y_axis_locked = false;
            } else {
                data->x_axis_locked = false;
                data->y_axis_locked = true;
            }
        }
    }
    
    // 出力値の決定
    int16_t output_x = dx;
    int16_t output_y = dy;
    
    if (data->axis_locked) {
        if (data->x_axis_locked) {
            output_y = 0;
            if (config->track_remainders) {
                data->remainder_y = dy;
            }
        } else if (data->y_axis_locked) {
            output_x = 0;
            if (config->track_remainders) {
                data->remainder_x = dx;
            }
        }
    }
    
    // イベントの送信 - ZMKのinput processorとして次のprocessorに渡す
    if (output_x != 0) {
        input_report_rel(dev, INPUT_REL_X, output_x, false, K_FOREVER);
    }
    
    if (output_y != 0) {
        input_report_rel(dev, INPUT_REL_Y, output_y, false, K_FOREVER);
    }
    
    // sync イベントを送信
    if (evt->sync) {
        input_report_rel(dev, 0, 0, true, K_FOREVER);
    }
}

static int xy_snap_init(const struct device *dev) {
    struct xy_snap_data *data = dev->data;
    
    data->last_activity_time = k_uptime_get();
    xy_snap_reset_state(data);
    
    return 0;
}

#define XY_SNAP_INIT(inst)                                                                         \
    static const struct xy_snap_config xy_snap_config_##inst = {                                  \
        .idle_timeout_ms = DT_INST_PROP_OR(inst, idle_timeout_ms, 0),                            \
        .switch_threshold = DT_INST_PROP_OR(inst, switch_threshold, 50),                         \
        .initial_threshold = DT_INST_PROP_OR(inst, initial_threshold, 10),                       \
        .allow_axis_switch = DT_INST_PROP(inst, allow_axis_switch),                              \
        .track_remainders = DT_INST_PROP(inst, track_remainders),                                \
    };                                                                                             \
                                                                                                   \
    static struct xy_snap_data xy_snap_data_##inst;                                               \
                                                                                                   \
    INPUT_CALLBACK_DEFINE(NULL, xy_snap_input_handler);                                           \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(inst, xy_snap_init, NULL, &xy_snap_data_##inst,                       \
                          &xy_snap_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,       \
                          &input_api);

DT_INST_FOREACH_STATUS_OKAY(XY_SNAP_INIT) 
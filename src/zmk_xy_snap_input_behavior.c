#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zmk/input/input.h>
#include <zmk/input/input_processor.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// XY Snap input processor state
struct xy_snap_state {
    int64_t last_activity_time;
    bool axis_locked;
    bool x_axis_locked;
    bool y_axis_locked;
    bool allow_axis_switch;
    int32_t switch_threshold;
    int32_t idle_timeout_ms;
    int32_t initial_threshold;
};

// XY Snap input processor configuration
struct xy_snap_config {
    int32_t idle_timeout_ms;
    int32_t switch_threshold;
    int32_t initial_threshold;
    bool allow_axis_switch;
};

// XY Snap input processor data
struct xy_snap_data {
    struct xy_snap_state state;
};

// Input processor process function
static int xy_snap_process(const struct device *dev, struct input_event *event,
                          uint32_t param1, uint32_t param2,
                          struct input_event **result) {
    struct xy_snap_data *data = dev->data;
    const struct xy_snap_config *config = dev->config;
    struct xy_snap_state *state = &data->state;
    
    // Only process relative movement events
    if (event->type != INPUT_EV_REL) {
        *result = event;
        return 0;
    }
    
    int64_t current_time = k_uptime_get();
    
    // Check for timeout - reset axis lock if idle too long
    if (current_time - state->last_activity_time > config->idle_timeout_ms) {
        state->axis_locked = false;
        state->x_axis_locked = false;
        state->y_axis_locked = false;
        LOG_DBG("XY Snap: Axis lock reset due to timeout");
    }
    
    // Determine axis lock if not already locked
    if (!state->axis_locked) {
        int32_t abs_x = (event->code == INPUT_REL_X) ? 
                       (event->value < 0 ? -event->value : event->value) : 0;
        int32_t abs_y = (event->code == INPUT_REL_Y) ? 
                       (event->value < 0 ? -event->value : event->value) : 0;
        
        if (abs_x > config->initial_threshold || abs_y > config->initial_threshold) {
            if (abs_x > abs_y) {
                state->x_axis_locked = true;
                state->y_axis_locked = false;
                LOG_DBG("XY Snap: X axis locked");
            } else {
                state->x_axis_locked = false;
                state->y_axis_locked = true;
                LOG_DBG("XY Snap: Y axis locked");
            }
            state->axis_locked = true;
        }
    }
    
    // Handle axis switching if enabled
    if (config->allow_axis_switch && state->axis_locked) {
        int32_t abs_x = (event->code == INPUT_REL_X) ? 
                       (event->value < 0 ? -event->value : event->value) : 0;
        int32_t abs_y = (event->code == INPUT_REL_Y) ? 
                       (event->value < 0 ? -event->value : event->value) : 0;
        
        if (state->x_axis_locked && abs_y > config->switch_threshold) {
            state->x_axis_locked = false;
            state->y_axis_locked = true;
            LOG_DBG("XY Snap: Switched to Y axis");
        } else if (state->y_axis_locked && abs_x > config->switch_threshold) {
            state->x_axis_locked = true;
            state->y_axis_locked = false;
            LOG_DBG("XY Snap: Switched to X axis");
        }
    }
    
    // Apply axis filtering
    if (state->x_axis_locked && event->code == INPUT_REL_Y) {
        // Suppress Y movement when X axis is locked
        event->value = 0;
    } else if (state->y_axis_locked && event->code == INPUT_REL_X) {
        // Suppress X movement when Y axis is locked
        event->value = 0;
    }
    
    state->last_activity_time = current_time;
    *result = event;
    return 0;
}

// Input processor API
static const struct input_processor_api xy_snap_api = {
    .process = xy_snap_process,
};

// Device tree configuration parsing
static int xy_snap_init(const struct device *dev) {
    struct xy_snap_data *data = dev->data;
    
    // Initialize state
    data->state.last_activity_time = 0;
    data->state.axis_locked = false;
    data->state.x_axis_locked = false;
    data->state.y_axis_locked = false;
    
    LOG_DBG("XY Snap input processor initialized");
    return 0;
}

// Device tree macros
#define XY_SNAP_INST(n)                                                      \
    static struct xy_snap_data xy_snap_data_##n;                            \
    static const struct xy_snap_config xy_snap_config_##n = {               \
        .idle_timeout_ms = DT_INST_PROP_OR(n, idle_timeout_ms, 300),        \
        .switch_threshold = DT_INST_PROP_OR(n, switch_threshold, 50),       \
        .initial_threshold = DT_INST_PROP_OR(n, initial_threshold, 10),     \
        .allow_axis_switch = DT_INST_PROP(n, allow_axis_switch),            \
    };                                                                       \
    DEVICE_DT_INST_DEFINE(n, xy_snap_init, NULL, &xy_snap_data_##n,        \
                          &xy_snap_config_##n, POST_KERNEL,                 \
                          CONFIG_INPUT_PROCESSOR_INIT_PRIORITY, &xy_snap_api);

DT_INST_FOREACH_STATUS_OKAY(XY_SNAP_INST) 
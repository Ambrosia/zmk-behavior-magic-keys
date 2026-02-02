/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_magic_keys

// Dependencies
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

typedef enum {
    OP_NONE = 0,
    OP_MAGIC,
    OP_SKIP_MAGIC,
} kc_kind_t;

// The keycode pressed and whether it was the result of one of the magic keys
typedef struct {
    kc_kind_t kind;
    uint32_t kc;
} history_event;

struct behavior_magic_keys_data {
    // 0 is the most recently pressed key, 1 is the second most pressed key
    history_event history[2];
};

struct behavior_magic_keys_config {
    size_t magic_rules_len;
    size_t skip_magic_rules_len;
    const uint32_t *magic_rules;
    const uint32_t *skip_magic_rules;
};

static struct behavior_magic_keys_data magic_keys_data = {.history = {
                                                              {
                                                                  .kind = OP_NONE,
                                                                  .kc = 0,
                                                              },
                                                              {
                                                                  .kind = OP_NONE,
                                                                  .kc = 0,
                                                              },
                                                          }};

static const uint32_t magic_rules[] = DT_INST_PROP(0, magic_rules);
static const uint32_t skip_magic_rules[] = DT_INST_PROP(0, skip_magic_rules);

static const struct behavior_magic_keys_config magic_keys_config = {
    .magic_rules = magic_rules,
    .magic_rules_len = ARRAY_SIZE(magic_rules),
    .skip_magic_rules = skip_magic_rules,
    .skip_magic_rules_len = ARRAY_SIZE(skip_magic_rules),
};

bool recordable_key(uint32_t keycode) {
    if (keycode >= A && keycode <= Z) {
        return true;
    }

    if (keycode >= NUMBER_1 && keycode <= NUMBER_0) {
        return true;
    }

    switch (keycode) {
    case SPACE:
    case COMMA:
    case DOT:
    case MINUS:
    case SEMICOLON:
    case SLASH:
    case EQUAL:
    case SINGLE_QUOTE:
    case LEFT_BRACKET:
    case RIGHT_BRACKET:
    case BACKSLASH:
    case GRAVE:
        return true;
    default:
        return false;
    }
}

void record_key(uint32_t keycode) {
    if (!recordable_key(keycode)) {
        LOG_DBG("non recordable key pressed 0x%08x", keycode);
        return;
    }

    magic_keys_data.history[1] = magic_keys_data.history[0];

    LOG_DBG("recording key press 0x%08x", keycode);
    magic_keys_data.history[0].kind = OP_NONE;
    magic_keys_data.history[0].kc = keycode;
}

void update_last_key_kind(uint32_t keycode, kc_kind_t kind) {
    magic_keys_data.history[0].kind = kind;
}

static int handle_keycode_state_changed(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    // If the key was pressed, not released
    if (ev->state) {
        uint32_t keycode = ZMK_HID_USAGE(ev->usage_page, ev->keycode);
        record_key(keycode);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(behavior_magic_keys, handle_keycode_state_changed);
ZMK_SUBSCRIPTION(behavior_magic_keys, zmk_keycode_state_changed);

static int behavior_magic_keys_init(const struct device *dev) {
    const struct behavior_magic_keys_config *cfg = dev->config;

    if (cfg->magic_rules_len % 2 != 0) {
        LOG_WRN("magic rules length not even, last key will be ignored");
    }

    if (cfg->skip_magic_rules_len % 2 != 0) {
        LOG_WRN("skip magic rules length not even, last key will be ignored");
    }

    return 0;
}

static uint32_t apply_magic_rules(const struct behavior_magic_keys_config *cfg, uint32_t keycode) {
    // Look up the key in the magic rules. As the magic rules are defined as an array of keys
    // where an even index is the input key and the odd index is the output key after it,
    // the result is at i + 1.

    // Checking with i + 1 instead of i, this should stop us from accessing it when it may
    // be out of array bounds.
    for (int i = 0; i + 1 < cfg->magic_rules_len; i += 2) {
        LOG_DBG("magic rule: does keycode 0x%08x match 0x%08x", keycode, cfg->magic_rules[i]);
        if (cfg->magic_rules[i] == keycode) {
            LOG_DBG("magic rule: yes! 0x%08x matched", cfg->magic_rules[i + 1]);
            return cfg->magic_rules[i + 1];
        }
    }

    LOG_DBG("magic rule: keycode 0x%08x not found in magic rule, repeating", keycode);
    return keycode;
}

static uint32_t apply_skip_magic_rules(const struct behavior_magic_keys_config *cfg,
                                       uint32_t keycode) {
    // Look up the key in the magic rules. As the magic rules are defined as an array of keys
    // where an even index is the input key and the odd index is the output key after it,
    // the result is at i + 1.

    // Checking with i + 1 instead of i, this should stop us from accessing it when it may
    // be out of array bounds.
    for (int i = 0; i + 1 < cfg->skip_magic_rules_len; i += 2) {
        LOG_DBG("skip magic rule: does keycode 0x%08x match 0x%08x", keycode,
                cfg->skip_magic_rules[i]);
        if (cfg->skip_magic_rules[i] == keycode) {
            LOG_DBG("skip magic rule: yes! 0x%08x matched", cfg->skip_magic_rules[i + 1]);
            return cfg->skip_magic_rules[i + 1];
        }
    }

    LOG_DBG("skip magic rule: keycode 0x%08x not found in skip magic rule, repeating", keycode);
    return keycode;
}

static int on_behavior_magic_keys_binding_pressed(struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event) {
    // 0 if magic, 1 if skip magic
    int historyIndex = binding->param1;

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_magic_keys_data *data = dev->data;
    const struct behavior_magic_keys_config *cfg = dev->config;

    if (historyIndex == 0) {
        // Magic key logic

        // The target key is the last key pushed, but if it's skip magic,
        // the target key is actually the one before that.
        LOG_DBG("history[0].kind %d == OP_SKIP_MAGIC %d", data->history[0].kind, OP_SKIP_MAGIC);
        history_event target =
            data->history[0].kind == OP_SKIP_MAGIC ? data->history[1] : data->history[0];
        uint32_t input = target.kc;
        if (data->history[0].kind == OP_SKIP_MAGIC) {
            LOG_DBG("magic key pressed, last key was skip magic, using second to last key 0x%08x "
                    "as input",
                    input);
        } else {
            LOG_DBG("magic key pressed, using last key 0x%08x as input", input);
        }

        // Run the magic rules for the last key pushed using the input,
        // unless it was the skip magic key, in which case the skip magic rules will be run instead.
        uint32_t result = data->history[0].kind == OP_SKIP_MAGIC
                              ? apply_skip_magic_rules(cfg, input)
                              : apply_magic_rules(cfg, input);

        LOG_DBG("magic rule applied, keycode 0x%08x -> 0x%08x", input, result);

        // This fires an event, recording the key press if it's recordable.
        raise_zmk_keycode_state_changed_from_encoded(result, true, event.timestamp);
        raise_zmk_keycode_state_changed_from_encoded(result, false, event.timestamp);

        // Update the history of the last key press, marking it as a result of the magic key
        // if it's recordable.
        update_last_key_kind(result, OP_MAGIC);
    } else if (historyIndex == 1) {
        // Skip magic key logic

        // The target key is the key before the last key pushed, but if it's magic,
        // the target key is actually the last key pushed.
        LOG_DBG("history[1].kind %d == OP_MAGIC %d", data->history[1].kind, OP_MAGIC);
        history_event target =
            data->history[1].kind == OP_MAGIC ? data->history[0] : data->history[1];
        uint32_t input = target.kc;
        if (data->history[0].kind == OP_MAGIC) {
            LOG_DBG("skip magic key pressed, second to last key was magic, using last key 0x%08x "
                    "as input",
                    input);
        } else {
            LOG_DBG("skip magic key pressed, using second to last key 0x%08x as input", input);
        }

        // Run the skip magic rules for the key before the last key pushed,
        // unless it was the magic key, in which case the magic rules will be run instead.
        uint32_t result = data->history[1].kind == OP_MAGIC ? apply_magic_rules(cfg, input)
                                                            : apply_skip_magic_rules(cfg, input);

        LOG_DBG("skip magic rule applied, keycode 0x%08x -> 0x%08x", input, result);

        // This fires an event, recording the key press if it's recordable.
        raise_zmk_keycode_state_changed_from_encoded(result, true, event.timestamp);
        raise_zmk_keycode_state_changed_from_encoded(result, false, event.timestamp);

        // Update the history of the last key press, marking it as a result of the skip magic key
        // if it's recordable.
        update_last_key_kind(result, OP_SKIP_MAGIC);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_behavior_magic_keys_binding_released(struct zmk_behavior_binding *binding,
                                                   struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api magic_keys_driver_api = {
    .binding_pressed = on_behavior_magic_keys_binding_pressed,
    .binding_released = on_behavior_magic_keys_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_magic_keys_init, NULL, &magic_keys_data, &magic_keys_config,
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &magic_keys_driver_api);

#endif

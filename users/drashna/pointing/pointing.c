// Copyright 2021 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "pointing.h"

static uint16_t mouse_debounce_timer = 0;
bool            enable_acceleration  = false;

#ifdef TAPPING_TERM_PER_KEY
#    define TAP_CHECK get_tapping_term(KC_BTN1, NULL)
#else
#    ifndef TAPPING_TERM
#        define TAPPING_TERM 200
#    endif
#    define TAP_CHECK TAPPING_TERM
#endif

__attribute__((weak)) void pointing_device_init_keymap(void) {}

void pointing_device_init_user(void) {
    set_auto_mouse_layer(_MOUSE);
    set_auto_mouse_enable(true);

    pointing_device_init_keymap();
}

__attribute__((weak)) report_mouse_t pointing_device_task_keymap(report_mouse_t mouse_report) {
    return mouse_report;
}

bool is_pointer_user(report_mouse_t* mouse_report) {
    bool moves   = (mouse_report->x != 0 && mouse_report->y != 0) || mouse_report->h != 0 || mouse_report->v != 0;
    bool toggles = false;

#if defined(KEYBOARD_bastardkb_charybdis)
    toggles = charybdis_get_pointer_sniping_enabled() || charybdis_get_pointer_dragscroll_enabled();
#endif

    return moves || toggles;
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    mouse_xy_report_t x = mouse_report.x, y = mouse_report.y;
    bool              pointer_used = is_pointer_user(&mouse_report);

    mouse_report.x = 0;
    mouse_report.y = 0;

    if (pointer_used && (timer_elapsed(mouse_debounce_timer) > TAP_CHECK)) {
        mouse_timer = timer_read();
#ifdef OLED_ENABLE
        oled_timer_reset();
#endif
        if (enable_acceleration) {
            x = (mouse_xy_report_t)(x > 0 ? x * x / 16 + x : -x * x / 16 + x);
            y = (mouse_xy_report_t)(y > 0 ? y * y / 16 + y : -y * y / 16 + y);
        }
        mouse_report.x = x;
        mouse_report.y = y;
    }

    return pointing_device_task_keymap(mouse_report);
}

bool process_record_pointing(uint16_t keycode, keyrecord_t* record) {
    switch (keycode) {
        case TT(_MOUSE):
            if (record->event.pressed) {
                mouse_keycode_tracker++;
            } else {
#if TAPPING_TOGGLE != 0
                if (record->tap.count == TAPPING_TOGGLE) {
                    tap_toggling ^= 1;
#    if TAPPING_TOGGLE == 1
                    if (!tap_toggling) mouse_keycode_tracker -= record->tap.count + 1;
#    else
                    if (!tap_toggling) mouse_keycode_tracker -= record->tap.count;
#    endif
                } else {
                    mouse_keycode_tracker--;
                }
#endif
            }
            mouse_timer = timer_read();
            break;
        case TG(_MOUSE):
            if (record->event.pressed) {
                tap_toggling ^= 1;
            }
            break;
        case MO(_MOUSE):
#if defined(KEYBOARD_ploopy)
        case DPI_CONFIG:
#elif (defined(KEYBOARD_bastardkb_charybdis) || defined(KEYBOARD_handwired_tractyl_manuform)) && !defined(NO_CHARYBDIS_KEYCODES)
        case SAFE_RANGE ...(CHARYBDIS_SAFE_RANGE - 1):
#endif
        case KC_MS_UP ... KC_MS_WH_RIGHT:
            record->event.pressed ? mouse_keycode_tracker++ : mouse_keycode_tracker--;
            mouse_timer = timer_read();
            break;
>>>>>>> 781df43890 (keymap changes, keep mouse layer when scrolled)
        case KC_ACCEL:
            enable_acceleration = record->event.pressed;
            break;
        default:
            mouse_debounce_timer = timer_read();
            break;
    }
    return true;
}

layer_state_t layer_state_set_pointing(layer_state_t state) {
    if (layer_state_cmp(state, _GAMEPAD) || layer_state_cmp(state, _DIABLO) || layer_state_cmp(state, _DIABLOII)) {
        state |= ((layer_state_t)1 << _MOUSE);
        set_auto_mouse_enable(false); // auto mouse can be disabled any time during run time
    } else {
        set_auto_mouse_enable(true);
    }
    return state;
}

#if defined(POINTING_DEVICE_AUTO_MOUSE_ENABLE)
__attribute__((weak)) bool is_mouse_record_keymap(uint16_t keycode, keyrecord_t* record) {
    return false;
}

bool is_mouse_record_user(uint16_t keycode, keyrecord_t* record) {
    if (is_mouse_record_keymap(keycode, record)) {
        return true;
    }
    switch (keycode) {
#    if defined(KEYBOARD_ploopy)
        case DPI_CONFIG:
#    elif (defined(KEYBOARD_bastardkb_charybdis) || defined(KEYBOARD_handwired_tractyl_manuform)) && !defined(NO_CHARYBDIS_KEYCODES)
        case SAFE_RANGE ...(CHARYBDIS_SAFE_RANGE - 1):
#    elif (defined(KEYBOARD_bastardkb_dilemma) && !defined(NO_DILEMMA_KEYCODES))
        case SAFE_RANGE ...(DILEMMA_SAFE_RANGE - 1):
#    endif
        case KC_ACCEL:
            return true;
    }
    return false;
}
#endif

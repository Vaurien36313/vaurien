//
//
#include "print.h"
#include QMK_KEYBOARD_H

enum layers {
    _QWERTY,
    _LEFT,
    _RIGHT
};

#define LEFT MO(_LEFT)
#define RIGHT MO(_RIGHT)
#define DEBUG_MATRIX_SCAN_RATE
void keyboard_post_init_user(void) {
    debug_enable = true;
    debug_matrix = true;
}
void matrix_scan_user(void) {
    static matrix_row_t last[MATRIX_ROWS] = {0};

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        matrix_row_t current = matrix_get_row(row);
        matrix_row_t diff = current ^ last[row];

        if (diff) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                if (diff & (1 << col)) {
                    if (!(current & (1 << col))) {
                        uprintf("r%uc%u\n", row, col);
                    }
                }
            }
            last[row] = current;
        }
    }
}
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_QWERTY] = LAYOUT(
        KC_ESC,  KC_1, KC_2,    KC_3,    KC_4,    KC_5,         KC_6,    KC_7,   KC_8,    KC_9,   KC_0,    KC_MINS,
        KC_LALT,  KC_Q, KC_W,    KC_E,    KC_R,    KC_T,         KC_Y,    KC_U,   KC_I,    KC_O,   KC_P,    KC_LBRC,
        KC_CAPS, KC_A, KC_S,    KC_D,    KC_F,    KC_G,         KC_H,    KC_J,   KC_K,    KC_L,   KC_SCLN, KC_QUOT,
        KC_TAB, KC_Z, KC_X,    KC_C,    KC_V,    KC_B,         KC_N,    KC_M,   KC_COMM, KC_DOT, KC_SLSH, KC_BSLS,
                       KC_VOLD, KC_VOLU,                                         KC_MPLY, KC_MNXT,
                                         KC_LGUI, KC_BSPC,      KC_SPC,  KC_PENT,
                                         LEFT,   KC_LALT,       KC_LCTL, KC_LSFT,
                                         KC_LCTL, KC_SPC,       KC_DEL,  RIGHT
    ),

    [_LEFT] = LAYOUT(
         KC_F12,  KC_F1,  KC_F2,  KC_F3,  KC_F4,  KC_F5,       _______,_______,_______,_______,_______,_______,
        _______,_______,_______,  KC_UP,_______,_______,       _______,_______,_______,_______,_______,_______,
        KC_GRV, _______,KC_LEFT,KC_DOWN,KC_RGHT,_______,       _______,_______,_______,_______,_______,_______,
        KC_LSFT,KC_PAUS,KC_BRID,KC_BRIU,KC_CALC,KC_PSCR,       _______,_______,_______,_______,_______,_______,
                        KC_MUTE,_______,                                       _______,_______,
                                        _______, KC_DEL,       _______,_______,
                                        _______,_______,       _______,_______,
                                        _______,_______,       _______,_______

    ),

    [_RIGHT] = LAYOUT(
          _______,_______,_______,_______,_______,_______,         KC_F6,  KC_F7,  KC_F8, KC_F9,  KC_F10, KC_F11,
          _______,_______,_______,_______,_______,_______,       KC_PSLS,   KC_P7,   KC_P8,   KC_P9, KC_EQL,KC_RBRC,
          _______,_______,_______,_______,_______,_______,       KC_PAST,   KC_P4,   KC_P5,   KC_P6, KC_NUM,KC_QUOT,
          _______,_______,_______,_______,_______,_______,       KC_PMNS,   KC_P1,   KC_P2,   KC_P3,_______,KC_BSLS,
                          _______,_______,                                       KC_MPRV,_______,
                                          _______,_______,       KC_PPLS,_______,
                                          _______,_______,       KC_P0,  KC_PDOT,
                                          _______,_______,       _______,_______
    )
};

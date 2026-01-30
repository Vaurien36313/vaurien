#include QMK_KEYBOARD_H
#include "sendstring_italian.h"




// ----- LED pins -----
#define LED_BLUE1_PIN D2
#define LED_BLUE2_PIN D0
#define LED_BLUE3_PIN B4
#define LED_RED_PIN   B5

// ----- timing -----
#define PEDAL_MULTITAP_MS         800  // window to detect 1/2/3 taps
#define GROUP_HOLD_MS              500  // long-press on SEL_1/2/3 toggles group
#define PEDAL_HOLD_THRESHOLD_MS    200  // how long last tap must be held to start "hold keys"
#define MAX_HELD_KEYS                6

#if defined(__GNUC__)
#    define MAYBE_UNUSED __attribute__((unused))
#else
#    define MAYBE_UNUSED
#endif

// ---------- Hold spec + IF_HELD() plumbing ----------

typedef struct {
    uint8_t  n;
    uint16_t keys[MAX_HELD_KEYS];
} hold_spec_t;

static inline hold_spec_t hold_none(void) {
    hold_spec_t s = {0};
    return s;
}

static inline hold_spec_t hold_make(const uint16_t *k, uint8_t n) {
    hold_spec_t s = {0};
    if (n > MAX_HELD_KEYS) n = MAX_HELD_KEYS;
    s.n = n;
    for (uint8_t i = 0; i < n; i++) s.keys[i] = k[i];
    return s;
}

typedef enum {
    MACRO_EXEC  = 0,  // actually run tap actions
    MACRO_QUERY = 1   // only discover whether an IF_HELD() is present
} macro_mode_t;

static macro_mode_t g_macro_mode = MACRO_EXEC;
static hold_spec_t *g_hold_out   = NULL;

static MAYBE_UNUSED void tap_seq(const uint16_t *keys, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        tap_code16(keys[i]);
    }
}
//to hold logic
static bool     sel_pressed[3]    = {false, false, false};
static bool     sel_hold_fired[3] = {false, false, false};
static uint16_t sel_press_timer[3]= {0, 0, 0};
// Use inside run_cluster_macro():
//   if (taps == 3) IF_HELD(KC_LSFT);
// In QUERY mode it records keys to hold; in EXEC mode it taps them.
#define IF_HELD(...)                                                     \
    do {                                                                 \
        const uint16_t tmp[] = { __VA_ARGS__ };                          \
        const uint8_t  n = (uint8_t)(sizeof(tmp) / sizeof(tmp[0]));      \
        if (g_macro_mode == MACRO_QUERY) {                               \
            if (g_hold_out) *g_hold_out = hold_make(tmp, n);             \
        } else {                                                         \
            tap_seq(tmp, n);                                             \
        }                                                                \
    } while (0)

// Hold engine state
static bool       pedal_is_down     = false;
static bool       hold_armed        = false;
static bool       hold_active       = false;
static uint16_t   hold_timer        = 0;
static hold_spec_t pending_hold_spec = {0};
#define EXEC_ONLY(stmt) do { if (g_macro_mode == MACRO_EXEC) { stmt; } } while (0)
// ---------- Layers / clusters ----------

enum layer_ids {
    CL_0 = 0, CL_1, CL_2,
    CL_3,     CL_4, CL_5
};

enum custom_keycodes {
    SEL_1 = SAFE_RANGE,
    SEL_2,
    SEL_3,
    PEDAL,
    CL_TOG
};

// Current cluster is stored as the default layer (persistent)
static inline uint8_t get_current_cluster(void) {
    return (uint8_t)get_highest_layer(default_layer_state);
}


static inline uint8_t cluster_group(uint8_t cluster) { return cluster / 3; } // 0..1
static inline uint8_t cluster_pos(uint8_t cluster)   { return cluster % 3; } // 0..2

static void update_cluster_leds(uint8_t cluster) {
    uint8_t grp = cluster_group(cluster);
    uint8_t pos = cluster_pos(cluster);

    // Blue LEDs: one-hot position (1..3)
    writePinLow(LED_BLUE1_PIN);
    writePinLow(LED_BLUE2_PIN);
    writePinLow(LED_BLUE3_PIN);

    if (pos == 0) writePinHigh(LED_BLUE1_PIN);
    if (pos == 1) writePinHigh(LED_BLUE2_PIN);
    if (pos == 2) writePinHigh(LED_BLUE3_PIN);

    // Red LED: ON for group 0 (0..2), OFF for group 1 (3..5)
    if (grp == 0) writePinLow(LED_RED_PIN);
    else          writePinHigh(LED_RED_PIN);
}

static void set_cluster(uint8_t cluster) {
    if (cluster > CL_5) cluster = CL_0;
    default_layer_set(1UL << cluster);
    update_cluster_leds(cluster);
}

// ---------- QMK init hooks ----------

void matrix_init_user(void) {
    // A2 toggle button: input with pull-up (active low)
    //--------------------------------------------------------------
}

void keyboard_post_init_user(void) {
    setPinOutput(LED_BLUE1_PIN);
    setPinOutput(LED_BLUE2_PIN);
    setPinOutput(LED_BLUE3_PIN);
    setPinOutput(LED_RED_PIN);

    update_cluster_leds(get_current_cluster());
}

layer_state_t layer_state_set_user(layer_state_t state) {
    (void)state;
    update_cluster_leds(get_current_cluster());
    return state;
}

// ---------- Keymap ----------
// Keep your working physical order mapping as-is.
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [CL_0] = LAYOUT(PEDAL, SEL_1, SEL_2, SEL_3, CL_TOG),
    [CL_1] = LAYOUT(PEDAL, SEL_1, SEL_2, SEL_3, CL_TOG),
    [CL_2] = LAYOUT(PEDAL, SEL_1, SEL_2, SEL_3, CL_TOG),
    [CL_3] = LAYOUT(PEDAL, SEL_1, SEL_2, SEL_3, CL_TOG),
    [CL_4] = LAYOUT(PEDAL, SEL_1, SEL_2, SEL_3, CL_TOG),
    [CL_5] = LAYOUT(PEDAL, SEL_1, SEL_2, SEL_3, CL_TOG),
};

// ---------- Pedal + toggle state ----------

static uint8_t  pedal_taps        = 0;
static uint32_t pedal_last_tap_ms = 0;



// ---------- Macro helper voids (kept, MAYBE_UNUSED) ----------

static MAYBE_UNUSED void ALT_TAB(void) {
    wait_ms(120);
    register_code(KC_LALT);
    wait_ms(120);
    tap_code(KC_TAB);
    wait_ms(120);
    unregister_code(KC_LALT);
    wait_ms(120);
}

static MAYBE_UNUSED void NEXT_PLAY(void) {
    tap_code16(KC_MNXT);
    tap_code16(LSFT(KC_N));
}

static MAYBE_UNUSED void PUSH_TALK(void)   { tap_code16(KC_F22); }
static MAYBE_UNUSED void STOP_MUS(void)    { tap_code16(KC_MPLY); }
static MAYBE_UNUSED void MUTE_JACK(void)   { tap_code16(KC_MPLY);tap_code16(KC_F21);}
static MAYBE_UNUSED void DEEJ_RUN(void)    { tap_code16(KC_F20); }
static MAYBE_UNUSED void DESK_TOP(void)    { tap_code16(LGUI(KC_D)); }
static MAYBE_UNUSED void CANCEL_CODE(void) { tap_code16(LCTL(KC_PAUS)); }
static MAYBE_UNUSED void BUILD_CODE(void)  { tap_code16(LCTL(KC_B)); }
static MAYBE_UNUSED void AI_BUILD(void)    { tap_code16(LCTL(LALT(KC_A))); }
static MAYBE_UNUSED void WIN_TOGGLE(void)  { tap_code16(KC_F19); }
static MAYBE_UNUSED void MOUSE_CLICK(void) { tap_code16(MS_BTN1); }
static MAYBE_UNUSED void YT_SKIP(void)     { tap_code16(LCTL(KC_RGHT)); }
static MAYBE_UNUSED void SCREENSHOT(void)  { tap_code16(LGUI(KC_R)); wait_ms(500); SEND_STRING("C:/Users/madda/Pictures/Screenshots\n");}
static MAYBE_UNUSED void PROGRAMMI_OC(void){ tap_code16(LGUI(KC_R)); wait_ms(500); SEND_STRING("C:/Users/madda/Documents/programmi occasionali\n");}
static MAYBE_UNUSED void DOWNLOADS(void)   { tap_code16(LGUI(KC_R)); wait_ms(500); SEND_STRING("C:/Users/madda/Downloads\n");}
static MAYBE_UNUSED void SE_womp(void)     { tap_code16(LGUI(KC_R)); wait_ms(500); SEND_STRING("C:/Users/madda/Documents/keyboards/sound effects/womp_womp.webm\n"); wait_ms(5000);tap_code16(LALT(KC_F4));}
static MAYBE_UNUSED void SE_vine(void)     { tap_code16(LGUI(KC_R)); wait_ms(500); SEND_STRING("C:/Users/madda/Documents/keyboards/sound effects/vine_boom.webm\n"); wait_ms(5000);tap_code16(LALT(KC_F4));}
static MAYBE_UNUSED void SE_RAAA(void)     { tap_code16(LGUI(KC_R)); wait_ms(500); SEND_STRING("C:/Users/madda/Documents/keyboards/sound effects/RAAA.mp3\n"); wait_ms(1500);tap_code16(LALT(KC_F4));}
static MAYBE_UNUSED void SE_pipe(void)     { tap_code16(LGUI(KC_R)); wait_ms(500); SEND_STRING("C:/Users/madda/Documents/keyboards/sound effects/metal_pipe.webm\n");wait_ms(5000);tap_code16(LALT(KC_F4));}

// ---------- Cluster macro mapping ----------
// You can put IF_HELD(...) directly in here.
static void run_cluster_macro(uint8_t cluster, uint8_t taps) {
    switch (cluster) {
        case CL_0://GENERAL
            if (taps == 1) 
                EXEC_ONLY(STOP_MUS());
            if (taps == 2) 
                EXEC_ONLY(WIN_TOGGLE());
            if (taps == 3) 
                EXEC_ONLY(DEEJ_RUN());
            break;

        case CL_1://GAMING
            if (taps == 1) 
                EXEC_ONLY(MUTE_JACK());
            if (taps == 2) 
                EXEC_ONLY(NEXT_PLAY());
            if (taps == 3) 
                EXEC_ONLY(YT_SKIP());    
            break;

        case CL_2://CODING
            if (taps == 1) 
                EXEC_ONLY(CANCEL_CODE()); 
            if (taps == 2) 
                EXEC_ONLY(BUILD_CODE());
            if (taps == 3) 
                EXEC_ONLY(AI_BUILD());
            break;

        case CL_3://FOLDERS
            if (taps == 1) 
                EXEC_ONLY(DOWNLOADS());
            if (taps == 2) 
                EXEC_ONLY(PROGRAMMI_OC());
            if (taps == 3) 
                EXEC_ONLY(SCREENSHOT());
            break;

        case CL_4://SOUNDS
            if (taps == 1) 
                EXEC_ONLY(SE_RAAA());
            if (taps == 2) 
                EXEC_ONLY(SE_pipe());
            if (taps == 3) 
                EXEC_ONLY(SE_womp());
            break;

        case CL_5://BLENDER
            if (taps == 1) 
                IF_HELD(KC_LSFT);
            if (taps == 2) 
                EXEC_ONLY(tap_code16(KC_Y));
            if (taps == 3) 
                IF_HELD(KC_LSFT);
            break;
    }
}

// ---------- Matrix scan ----------

void matrix_scan_user(void) {

    // --- Fire SEL holds while still pressed ---
    for (uint8_t idx = 0; idx < 3; idx++) {
        if (sel_pressed[idx] && !sel_hold_fired[idx] &&
            timer_elapsed(sel_press_timer[idx]) >= GROUP_HOLD_MS) {

            uint8_t cur = get_current_cluster();
            uint8_t grp = cluster_group(cur);
            uint8_t pos = idx;               // go to the pressed button position
            grp = 1 - grp;                   // toggle group

            set_cluster((grp * 3) + pos);

            sel_hold_fired[idx] = true;      // prevent re-firing + suppress tap on release
        }
    }
    // If armed and still held, start holding the requested keys
    if (hold_armed && pedal_is_down && !hold_active) {
        if (timer_elapsed(hold_timer) >= PEDAL_HOLD_THRESHOLD_MS) {
            for (uint8_t i = 0; i < pending_hold_spec.n; i++) {
                register_code16(pending_hold_spec.keys[i]);
            }
            hold_active = true;
        }
    }

    // A2 toggle (F5) -> flips group while keeping position
    

    // Commit tap macro after timeout (only if not doing/arming a hold)
    if (!hold_active && !hold_armed &&
        pedal_taps != 0 &&
        timer_elapsed32(pedal_last_tap_ms) >= PEDAL_MULTITAP_MS) {
        if (pedal_taps>6) pedal_taps=6;
        uint8_t cluster = get_current_cluster();
        uint8_t taps = pedal_taps;
        if (taps < 1) taps = 1;
        if (taps > 3) {
            uint8_t grp = cluster_group(cluster);
            uint8_t pos = cluster_pos(cluster);
            cluster = ((1 - grp) * 3) + pos;  // same pos, other group
            taps    = taps - 3;               // 4..6 -> 1..3
        }
        run_cluster_macro(cluster, taps);

        pedal_taps = 0;
        pedal_last_tap_ms = 0;
    }
}

// ---------- Key handling ----------

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
    case CL_TOG:
    if (record->event.pressed) {
        uint8_t cur = get_current_cluster();
        uint8_t grp = cluster_group(cur);
        uint8_t pos = cluster_pos(cur);

        grp = (grp == 0) ? 1 : 0;
        set_cluster((grp * 3) + pos);
    }
    return false;
        case SEL_1:
        case SEL_2:
        case SEL_3: {
            uint8_t idx = (keycode == SEL_1) ? 0 : (keycode == SEL_2) ? 1 : 2;
        
            if (record->event.pressed) {
                sel_pressed[idx]     = true;
                sel_hold_fired[idx]  = false;
                sel_press_timer[idx] = timer_read();
            } else {
                sel_pressed[idx] = false;
        
                if (!sel_hold_fired[idx]) {
                    // TAP action (short press): select position within current group
                    uint8_t cur = get_current_cluster();
                    uint8_t grp = cluster_group(cur);
                    uint8_t pos = idx;
                    set_cluster((grp * 3) + pos);
                }
                // else: hold already fired in matrix_scan_user, do nothing on release
            }
            return false;
        }

        case PEDAL:
            if (record->event.pressed) {
                pedal_is_down = true;

                if (pedal_taps < 6) pedal_taps++;
                pedal_last_tap_ms = timer_read32();

                // Query run_cluster_macro to see if this (cluster,taps) contains an IF_HELD(...)
                hold_spec_t hs = hold_none();
                g_macro_mode = MACRO_QUERY;
                g_hold_out = &hs;

                run_cluster_macro(get_current_cluster(), pedal_taps);

                g_hold_out = NULL;
                g_macro_mode = MACRO_EXEC;

                if (hs.n > 0) {
                    pending_hold_spec = hs;
                    hold_armed = true;
                    hold_timer = timer_read();
                } else {
                    hold_armed = false;
                    pending_hold_spec = hold_none();
                }

            } else {
                pedal_is_down = false;

                // release held keys if we were holding
                bool was_holding = hold_active;
                if (was_holding) {
                    for (uint8_t i = 0; i < pending_hold_spec.n; i++) {
                        unregister_code16(pending_hold_spec.keys[i]);
                    }
                    hold_active = false;
                }

                // releasing pedal ends armed state
                hold_armed = false;
                pending_hold_spec = hold_none();

                // if we were holding, cancel queued tap macro so nothing fires after release
                if (was_holding) {
                    pedal_taps = 0;
                    pedal_last_tap_ms = 0;
                }
            }
            return false;

        default:
            return true;
    }
}

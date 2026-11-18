#include "wls.h"

static ioline_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;

/* The right half uses its own column pins. */
#if defined(SPLIT_KEYBOARD) && defined(MATRIX_COL_PINS_RIGHT)
static ioline_t col_pins_right[MATRIX_COLS] = MATRIX_COL_PINS_RIGHT;
#else
#    define col_pins_right col_pins
#endif

/* Set once the wireless module has been told to switch its radio off. */
bool hs_radio_parked = false;

void usart_init(void);

/*
 * Before sleeping in a wireless mode the stock firmware releases the split
 * serial pins on both halves, so neither half drives the link while asleep.
 */
void hs_split_link_park(void) {
#ifdef SPLIT_KEYBOARD
    if (lower_sleep || (wireless_get_current_devs() != DEVS_USB)) {
        gpio_set_pin_input(SERIAL_USART_RX_PIN);
        gpio_set_pin_input(SERIAL_USART_TX_PIN);
    }
#endif
}

/* Hand the split serial pins back to the USART. */
void hs_split_link_restore(void) {
#ifdef SPLIT_KEYBOARD
    usart_init();
#endif
}

/* Ask the wireless module to switch its radio off and let it do so. */
static void hs_radio_off(void) {
    uint32_t timer = timer_read32();

    md_send_devctrl(MD_SND_CMD_DEVCTRL_USB);
    while (smsg_is_busy() && timer_elapsed32(timer) < 500) {
        md_main_task();
    }
    wait_ms(HS_RADIO_OFF_DELAY);
    hs_radio_parked = true;
}

bool hs_modeio_detection(bool update, uint8_t *mode, uint8_t lsat_btdev) {
    static uint32_t scan_timer = 0x00;

    if ((update != true) && (timer_elapsed32(scan_timer) <= (HS_MODEIO_DETECTION_TIME))) {
        return false;
    }
    scan_timer = timer_read32();
#if defined(HS_BT_DEF_PIN) && defined(HS_2G4_DEF_PIN)
    uint8_t now_mode         = 0x00;
    uint8_t hs_mode          = 0x00;
    static uint8_t last_mode = 0x00;
    bool sw_mode             = false;
    now_mode                 = (HS_GET_MODE_PIN(HS_USB_PIN_STATE) ? 3 : (HS_GET_MODE_PIN(HS_BT_PIN_STATE) ? 1 : ((HS_GET_MODE_PIN(HS_2G4_PIN_STATE) ? 2 : 0))));
    hs_mode                  = (*mode >= DEVS_BT1 && *mode <= DEVS_BT5) ? 1 : ((*mode == DEVS_2G4) ? 2 : ((*mode == DEVS_USB) ? 3 : 0));
    sw_mode                  = ((update || (last_mode == now_mode)) && (hs_mode != now_mode)) ? true : false;
    last_mode                = now_mode;

    switch (now_mode) {
        case 1:
            *mode = hs_bt;
            if (sw_mode) {
                wireless_devs_change(wireless_get_current_devs(), lsat_btdev, false);
            }
            break;
        case 2:
            *mode = hs_2g4;
            if (sw_mode) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_2G4, false);
            }
            break;
        case 3:
            *mode = hs_usb;
            if (sw_mode)
                wireless_devs_change(wireless_get_current_devs(), DEVS_USB, false);

            break;
        default:
            break;
    }

    if (sw_mode) {
        hs_rgb_blink_set_timer(timer_read32());
        suspend_wakeup_init();
        return true;
    }
#else
    *mode = hs_none;
#endif

    return false;
}

static uint32_t hs_linker_rgb_timer = 0x00;

bool hs_mode_scan(bool update, uint8_t moude, uint8_t lsat_btdev) {

    if (hs_modeio_detection(update, &moude, lsat_btdev)) {

        return true;
    }
    hs_rgb_blink_hook();
    return false;
}

void hs_rgb_blink_set_timer(uint32_t time) {
    hs_linker_rgb_timer = time;
}

uint32_t hs_rgb_blink_get_timer(void) {
    return hs_linker_rgb_timer;
}

bool hs_rgb_blink_hook() {
    static uint8_t last_status;

    if (last_status != *md_getp_state()) {
        last_status = *md_getp_state();
        hs_rgb_blink_set_timer(0x00);
    }

    switch (*md_getp_state()) {
        case MD_STATE_NONE: {
            hs_rgb_blink_set_timer(0x00);
        } break;

        case MD_STATE_DISCONNECTED:
            if (hs_rgb_blink_get_timer() == 0x00) {
                hs_rgb_blink_set_timer(timer_read32());
                extern void wireless_devs_change_kb(uint8_t old_devs, uint8_t new_devs, bool reset);
                wireless_devs_change_kb(wireless_get_current_devs(), wireless_get_current_devs(), false);
            } else {
                if (timer_elapsed32(hs_rgb_blink_get_timer()) >= HS_LBACK_TIMEOUT) {
                    hs_rgb_blink_set_timer(timer_read32());
                    md_send_devctrl(MD_SND_CMD_DEVCTRL_USB);
                    wait_ms(200);
                    lpwr_set_timeout_manual(true);
                }
            }
        case MD_STATE_CONNECTED: {
            uint32_t sleep_timeout = lpwr_timeout_value_read();
            if (hs_rgb_blink_get_timer() == 0x00) {
                hs_rgb_blink_set_timer(timer_read32());
            } else if (sleep_timeout && (timer_elapsed32(hs_rgb_blink_get_timer()) >= sleep_timeout)) {
                hs_rgb_blink_set_timer(timer_read32());
                lpwr_set_timeout_manual(true);
            }
        } break;
        default:
            break;
    }
    return true;
}

void lpwr_exti_init_hook(void) {

    /* Only the half with the wireless module has the mode switch. */
    if (is_keyboard_master()) {
#ifdef HS_BT_DEF_PIN
        gpio_set_pin_input_high(HS_BT_DEF_PIN);
        waitInputPinDelay();
        palEnableLineEvent(HS_BT_DEF_PIN, PAL_EVENT_MODE_BOTH_EDGES);
#endif

#ifdef HS_2G4_DEF_PIN
        gpio_set_pin_input_high(HS_2G4_DEF_PIN);
        waitInputPinDelay();
        palEnableLineEvent(HS_2G4_DEF_PIN, PAL_EVENT_MODE_BOTH_EDGES);
#endif
    }

    if (lower_sleep) {
#if DIODE_DIRECTION == ROW2COL
        ioline_t *cols = is_keyboard_left() ? col_pins : col_pins_right;

        for (uint8_t i = 0; i < MATRIX_COLS; i++) {
            if (cols[i] != NO_PIN) {
                gpio_set_pin_output(cols[i]);
                gpio_write_pin_high(cols[i]);
            }
        }
#endif
    } else if (!is_keyboard_left()) {
        /* Turning the knob wakes the keyboard from normal sleep. */
        gpio_set_pin_input_high(HS_ENCODER_WAKE_PIN);
        waitInputPinDelay();
        palEnableLineEvent(HS_ENCODER_WAKE_PIN, PAL_EVENT_MODE_BOTH_EDGES);
    }

    /*
     * Cable detection only exists on the half with the USB port. On the
     * other half the pin is not driven and must not be a wake-up source.
     */
    if (is_keyboard_master()) {
        gpio_set_pin_input(HS_BAT_CABLE_PIN);
        waitInputPinDelay();
        palEnableLineEvent(HS_BAT_CABLE_PIN, PAL_EVENT_MODE_RISING_EDGE);
    }

#ifdef SPLIT_KEYBOARD
    /*
     * The halves wake each other through the split serial link: the half
     * that wakes first drives its TX pin again, which the other half sees
     * as an edge on RX. Same configuration as the stock firmware.
     */
    gpio_set_pin_input(SERIAL_USART_RX_PIN);
    waitInputPinDelay();
    palEnableLineEvent(SERIAL_USART_RX_PIN, PAL_EVENT_MODE_BOTH_EDGES);
#endif
}

/* Runs right after stop mode, once the clocks are back. */
void lpwr_clock_enable_user(void) {
    hs_split_link_restore();
}

/*
 * In USB mode the MCU keeps running while asleep (lights off only) on the
 * half without the USB port, and on the other half while the cable is in.
 */
bool lpwr_is_allow_enter_stop_hook(void) {

    if (!lower_sleep && (wireless_get_current_devs() == DEVS_USB)) {
        if (!is_keyboard_master() || gpio_read_pin(HS_BAT_CABLE_PIN)) {
            return false;
        }
    }

    return true;
}

void palcallback_cb(uint8_t line) {
    switch (line) {
        case PAL_PAD(HS_BAT_CABLE_PIN): {
            lpwr_set_sleep_wakeupcd(LPWR_WAKEUP_CABLE);
        } break;
#ifdef HS_BT_DEF_PIN
        case PAL_PAD(HS_2G4_DEF_PIN): {
            lpwr_set_sleep_wakeupcd(LPWR_WAKEUP_SWITCH);
        } break;
#endif

#ifdef HS_2G4_DEF_PIN
        case PAL_PAD(HS_BT_DEF_PIN): {
            lpwr_set_sleep_wakeupcd(LPWR_WAKEUP_SWITCH);
        } break;
#endif
        default: {

        } break;
    }
}

void lpwr_stop_hook_pre(void) {

    gpio_write_pin_low(LED_POWER_EN_PIN);
    gpio_write_pin_low(LED_POWER_EN2_PIN);

    /*
     * Radio off before stop mode, once per sleep: always for the low-battery
     * sleep, otherwise only when there is no host connection to keep alive.
     * A connected module is left to its own sleep handling.
     */
    if (is_keyboard_master() && !hs_radio_parked && (wireless_get_current_devs() != DEVS_USB)) {
        if (lower_sleep || (*md_getp_state() != MD_STATE_CONNECTED)) {
            hs_radio_off();
        }
    }

    if (lower_sleep) {
        lpwr_set_sleep_wakeupcd(LPWR_WAKEUP_UART);
    }
}

void lpwr_stop_hook_post(void) {
    if (lower_sleep) {
        if (!is_keyboard_master() && (lpwr_get_sleep_wakeupcd() != LPWR_WAKEUP_NONE)) {
            lower_sleep = false;
            lpwr_set_state(LPWR_WAKEUP);
            return;
        }
        switch (lpwr_get_sleep_wakeupcd()) {
            case LPWR_WAKEUP_SWITCH:
                if (!manual_off) {
                    lpwr_set_state(LPWR_STOP);
                    break;
                }
                /* fall through */
            case LPWR_WAKEUP_USB:
            case LPWR_WAKEUP_CABLE: {
                lower_sleep = false;
                manual_off  = false;
                lpwr_set_state(LPWR_WAKEUP);
            } break;
            default: {
                lpwr_set_state(LPWR_STOP);
            } break;
        }
    }
}
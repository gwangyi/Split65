#include QMK_KEYBOARD_H

#include "quantum.h"
#include <stdbool.h>
#include "wireless.h"

#define HS_BT_DEF_PIN                     C14
#define HS_2G4_DEF_PIN                    C15
#define HS_BT_PIN_STATE                   0, 1
#define HS_2G4_PIN_STATE                  1, 0
#define HS_USB_PIN_STATE                  1, 1

#define HS_GET_MODE_PIN_(pin_bt, pin_2g4) ((((#pin_bt)[0] == 'x') || ((readPin(HS_BT_DEF_PIN) + 0x30) == ((#pin_bt)[0]))) && (((#pin_2g4)[0] == 'x') || ((readPin(HS_2G4_DEF_PIN) + 0x30) == ((#pin_2g4)[0]))))
#define HS_GET_MODE_PIN(state)            HS_GET_MODE_PIN_(state)
#define HS_MODEIO_DETECTION_TIME          50
#define HS_LBACK_TIMEOUT                  (30 * 1000)
#ifndef HS_SLEEP_TIMEOUT
#    define HS_SLEEP_TIMEOUT              (1 * 300000) // Default: 5 minutes
#endif

enum modeio_mode {
    hs_none = 0,
    hs_usb,
    hs_bt,
    hs_2g4,
    hs_wireless
};

/* Encoder pin armed as a wake-up source on the half that has the encoder. */
#ifndef HS_ENCODER_WAKE_PIN
#    define HS_ENCODER_WAKE_PIN           B6
#endif

/* Time given to the wireless module to act on "radio off" before stop mode. */
#ifndef HS_RADIO_OFF_DELAY
#    define HS_RADIO_OFF_DELAY            300
#endif

extern bool lower_sleep;
extern bool hs_radio_parked;

void hs_split_link_park(void);
void hs_split_link_restore(void);
bool hs_rgb_blink_hook(void);
bool hs_mode_scan(bool update, uint8_t moude, uint8_t lsat_btdev);
bool hs_modeio_detection(bool update, uint8_t *mode, uint8_t lsat_btdev);
void hs_rgb_blink_set_timer(uint32_t time);

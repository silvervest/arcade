#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "bsp/board.h"
#include "tusb.h"

#define SDA_PIN 0
#define SCL_PIN 1
#define BLK_PIN 2

/**
 * Values to control the TDA935x I2C RGB blanking setting
 */
const uint8_t tda935x_addr = 0x45;
const uint8_t tda935x_ctrl_0_addr = 0x2A;
const uint8_t tda935x_ctrl_0_cmd = 0x4C;

void hid_task(void);
void crt_init(void);
void crt_task(void);
void crt_rgb_enable(void);

int main() {
    stdio_init_all();
    crt_init();
    // board_init();
    // tusb_init();

    // we only seem to need to do this once at startup, once the tv has warmed up
    // give the system about 10 seconds to get past VGA post stuff not in 15khz then proceed
    sleep_ms(10000);
    crt_rgb_enable();

    sleep_ms(500);
    // start rgb blanking
    gpio_put(BLK_PIN, 1);

    while (1) {
        // tud_task();
        crt_task();
        // hid_task();
    }

}

/**
 * Init our CRT stuff
 * 
 */
void crt_init(void) {
    // I2C bus for use in communicating with the TDA935x jungle
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // and the blanking pin
    gpio_set_dir(BLK_PIN, GPIO_OUT);
    gpio_put(BLK_PIN, 0);
}

/**
 * Perform whatever actions we need to do for CRT maintenance
 */
void crt_task(void) {
    // only do stuff once every five seconds
    const uint32_t interval_ms = 5000;
    static uint32_t crt_start_ms = 0;

    if ((board_millis() - crt_start_ms) < interval_ms) return; // not enough time
    crt_start_ms = board_millis() + interval_ms;

    // every second, send the signal to enable rgb again, just in case
    crt_rgb_enable();
}

/**
 * Send our address and command over the I2C bus to enable RGB blanking
 */
void crt_rgb_enable(void) {
    uint8_t buf[2];
    buf[0] = tda935x_ctrl_0_addr;
    buf[1] = tda935x_ctrl_0_cmd;
    i2c_write_blocking(i2c_default, tda935x_addr, buf, 2, false);
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}


//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

void hid_task(void) {
    // Poll every 10ms
    const uint32_t interval_ms = 100;
    static uint32_t hid_start_ms = 0;

    if ((board_millis() - hid_start_ms) < interval_ms) return; // not enough time
    hid_start_ms = board_millis() + interval_ms;

    // Remote wakeup
    if (tud_suspended()) {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
    }

    /*------------- Player 1 -------------*/
    // if (tud_hid_n_ready(ITF_PLAYER_1)) {
    //     // set buttons
    //     // report.buttons = player1;
    //     // report.xy = player1_button;
    //     // report.buttons = player1_button;
    //     // player1_button = (player1_button + 1) & 0x07; // limit button to the range 0..7

    //     tud_hid_n_report(ITF_PLAYER_1, 0x00, &player1, sizeof(player1));
    // }

    // if (tud_hid_n_ready(ITF_PLAYER_2)) {
    //     // set buttons
    //     // report.xy = player2_button;
    //     // report.buttons = player2_button;
    //     // player2_button = (player2_button - 1) & 0x07; // limit button to the range 0..7
    //     // report.buttons = player2;

    //     tud_hid_n_report(ITF_PLAYER_2, 0x00, &player2, sizeof(player2));
    // }

}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    // TODO not Implemented
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    // TODO set LED based on CAPLOCK, NUMLOCK etc...
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

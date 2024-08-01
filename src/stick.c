#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "bsp/board.h"
#include "tusb.h"
#include <stdbool.h>

#define ATTRIBUTE_PACKED  __attribute__((packed, aligned(1)))

/**
 * Struct to hold our HID report data
 * This needs to be tightly packed and optimised in order to pass the correct bit fields defined in usb_descriptors.c
 */
typedef struct ATTRIBUTE_PACKED {
    uint8_t x;
    uint8_t y;
    bool a:1;
    bool b:1;
    bool c:1;
    bool d:1;
    bool e:1;
    bool f:1;
    bool g:1;
    bool h:1;
    bool i:1;
    bool j:1;
    bool k:1;
    bool l:1;
} buttons;

// init these variables with zero'd values
buttons player1 = {
    .x = 128,
    .y = 128,
    .a = 0,
    .b = 0,
    .c = 0,
    .d = 0,
    .e = 0,
    .f = 0,
    .g = 0,
    .h = 0,
    .i = 0,
    .j = 0,
    .k = 0,
    .l = 0
};
buttons player2 = {
    .x = 128,
    .y = 128,
    .a = 0,
    .b = 0,
    .c = 0,
    .d = 0,
    .e = 0,
    .f = 0,
    .g = 0,
    .h = 0,
    .i = 0,
    .j = 0,
    .k = 0,
    .l = 0
};

// on board led
#define LED_PIN     25

// details for i2c i/o expanders
#define SDA_PIN 0
#define SCL_PIN 1
#define P1_INT_PIN 2
#define P2_INT_PIN 3
const uint8_t p1_addr = 0x20;
const uint8_t p2_addr = 0x21;

void hid_task(void);
void exp_init(void);
void exp_read(uint8_t gpio, uint8_t *buf);
void exp_interrupt(uint gpio, uint32_t event_mask);
int64_t exp_alarm(alarm_id_t id, void *user_data);
void player_update(buttons *player, uint16_t state);

int main() {
    stdio_init_all();
    board_init();
    exp_init();
    tusb_init();

    while (1) {
        tud_task();
        hid_task();
        tight_loop_contents();
    }
}

/**
 * Initialise our PCF8575 I/O expanders and associated stuff
 */
void exp_init(void)
{
    // start i2c
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    // we have external pull-up so this is not necessary but whatever
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // each i/o expander has an interrupt pin
    gpio_set_dir(P1_INT_PIN, GPIO_IN);
    gpio_pull_up(P1_INT_PIN);
    gpio_set_irq_enabled_with_callback(P1_INT_PIN, GPIO_IRQ_EDGE_FALL, true, &exp_interrupt);
    gpio_set_dir(P2_INT_PIN, GPIO_IN);
    gpio_pull_up(P2_INT_PIN);
    gpio_set_irq_enabled_with_callback(P2_INT_PIN, GPIO_IRQ_EDGE_FALL, true, &exp_interrupt);

    // tell the i/o expander to 
    uint8_t all_ones[] = {0xff, 0xff};
    i2c_write_blocking(i2c_default, p1_addr, all_ones, 2, false);
    i2c_write_blocking(i2c_default, p2_addr, all_ones, 2, false);
}

/**
 * Read the two ports from the PCF8575 by requesting two bytes
 * This is pretty unsafe memory-wise, so use carefully!
 */
void exp_read(uint8_t gpio, uint8_t *buf)
{
    // read from the appropriate i2c address
    uint8_t addr;
    if (gpio == P1_INT_PIN) {
        addr = p1_addr;
    } else if (gpio == P2_INT_PIN) {
        addr = p2_addr;
    }

    // read two bytes from the expander
    i2c_read_blocking(i2c_default, addr, buf, 2, false);
}

/**
 * Interrupt handler for PCF8575 /INT pin
 * This is triggered when there's any change detected in any of the pins connected to the PCF
 * Used for debouncing, it will pass the PCF's port state to an alarm for checking
 */
void exp_interrupt(uint gpio, uint32_t event_mask)
{
    // just a number plucked out of thin air, but 5ms seems to be fine in practice
    const uint8_t debounce_time_ms = 5;

    // read two bytes from the expander
    uint8_t new_state[2] = {255,255};
    exp_read(gpio, new_state);

    // pack our state data and gpio pin into a uint to pass to the alarm
    uint32_t alarm_data = (new_state[1] << 16) | (new_state[0] << 8) | gpio;

    // set an alarm and send the gpio to the alarm handler
    add_alarm_in_ms(debounce_time_ms, exp_alarm, (void *)(uintptr_t)alarm_data, false);
}

/**
 * Alarm handler set by exp_interrupt()
 * Used for debouncing our button inputs
 */
int64_t exp_alarm(alarm_id_t id, void *user_data)
{
    // extract out our gpio and state data back out from the alarm data
    uint32_t data = (uintptr_t)user_data;
    uint gpio = data & 0xFF;
    uint16_t first_state = ((data >> 8) & 0xFFFF);

    // read two bytes from the expander
    uint8_t state[2];
    exp_read(gpio, state);
    uint16_t second_state = (state[1] << 8) | state[0];

    if (first_state ^ second_state) {
        // differences between interrupt state and alarm state, so ignore this bounce
        return 0;
    }

    // if we're here that means there's no differences, so we're stable - use this update!
    // since buttons pull down to ground, flip the inputs so they're a logical 0 or 1
    second_state = ~second_state;
    gpio_put(LED_PIN, second_state > 0); // shine our led if any buttons are pressed
    
    // update the relevant player's state
    if (gpio == P1_INT_PIN) {
        player_update(&player1, second_state);
    } else if (gpio == P2_INT_PIN) {
        player_update(&player2, second_state);
    }

    return 0;
}

/**
 * Update a player variable's state based on the port input data from our PCF8575's
 */
void player_update(buttons *player, uint16_t state)
{
    // handle the joystick inputs
    // range of 0 - 255, where 128 is "middle"
    if (state & (1 << 0)) { // up
        player->y = 0;
    } else if (state & (1 << 1)) { // down
        player->y = 255;
    } else { // neither
        player->y = 128;
    }
    if (state & (1 << 2)) { // left
        player->x = 0;
    } else if (state & (1 << 3)) { // right
        player->x = 255;
    } else { // neither
        player->x = 128;
    }

    // and all other buttons are simply extracting the bit values out
    player->a = state & (1 << 4);
    player->b = state & (1 << 5);
    player->c = state & (1 << 6);
    player->d = state & (1 << 7);
    player->e = state & (1 << 8);
    player->f = state & (1 << 9);
    player->g = state & (1 << 10);
    player->h = state & (1 << 11);
    player->i = state & (1 << 12);
    player->j = state & (1 << 13);
    player->k = state & (1 << 14);
    player->l = state & (1 << 15);
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    // enable gpio interrupts here perhaps?
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    // disable gpio interrupts here perhaps?
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
enum {
  ITF_PLAYER_1 = 0,
  ITF_PLAYER_2 = 1
};

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
    if (tud_hid_n_ready(ITF_PLAYER_1)) {
        tud_hid_n_report(ITF_PLAYER_1, 0x00, &player1, sizeof(player1));
    }
    /*------------- Player 2 -------------*/
    if (tud_hid_n_ready(ITF_PLAYER_2)) {
        tud_hid_n_report(ITF_PLAYER_2, 0x00, &player2, sizeof(player2));
    }

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

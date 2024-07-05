#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "bsp/board.h"
#include "tusb.h"
#include <stdbool.h>

#define ATTRIBUTE_PACKED  __attribute__((packed, aligned(1)))

/**
 * Struct to hold our HID report
 * This needs to be tightly packed and optimised in order to pass the correct bit fields
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
    bool start:1;
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
    .start = 0
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
    .start = 0
};

// on board led
#define LED_PIN     25

// player controls mapped to gpio
#define P1_UP       0
#define P1_DOWN     1
#define P1_LEFT     2
#define P1_RIGHT    3
#define P1_A        4
#define P1_B        5
#define P1_C        6
#define P1_D        7
#define P1_E        8
#define P1_F        9
#define P1_G        10
#define P1_H        11
#define P1_START    12
#define P2_UP       13
#define P2_DOWN     14
#define P2_LEFT     15
#define P2_RIGHT    16
#define P2_A        17
#define P2_B        18
#define P2_C        19
#define P2_D        20
#define P2_E        21
#define P2_F        22
#define P2_G        26
#define P2_H        27
#define P2_START    28

void hid_task(void);
void button_init(void);
void button_read(uint gpio, uint32_t event_mask);
int64_t button_alarm(alarm_id_t id, void *user_data);

int main() {
    stdio_init_all();
    board_init();
    button_init();
    tusb_init();

    while (1) {
        tud_task();
        hid_task();
    }
}

void button_init(void)
{
    // init all gpio as input, enable all pull-up resistors, and enable irq handler when rising/falling
    for (int i = 0; i < 29; i++) {
        if (i > 22 && i < 26) continue;
        gpio_init(i);
        gpio_set_dir(i, false);
        gpio_pull_up(i);
        gpio_set_irq_enabled_with_callback(i, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &button_read);
    }
}

/**
 * Handle our interrupt when a GPIO pin passes an edge rise/fall
 * 
 * For debouncing purposes, once we receive an interrupt we start a short timer that will check if the
 * button is in the same state, then handle the appropriate action
 */
void button_read(uint gpio, uint32_t event_mask)
{
    // just a number plucked out of thin air, but 5ms seems to be fine in practice
    const uint8_t debounce_time_ms = 5;

    // just shove our gpio into the existing 32-bit event_mask variable and pass that along to the alarm handler
    // event_mask |= gpio << 8;
    uint32_t alarm_data = (event_mask << 8) | gpio;
    add_alarm_in_ms(debounce_time_ms, button_alarm, (void *)(uintptr_t)alarm_data, false);
}

int64_t button_alarm(alarm_id_t id, void *user_data) {
    // extract out our gpio and event_mask back out from user data
    uint32_t data = (uintptr_t)user_data;
    uint8_t event_mask = ((data >> 8) & 0xFF);
    uint gpio = data & 0xFF;
    bool pressed = event_mask & GPIO_IRQ_EDGE_FALL;

    // check that gpio's current state is same as the interrupt
    // this is opposite because we're pulling down the GPIO to enable, so when gpio_cur == 1, pressed == 0 and vice versa
    uint8_t gpio_cur = gpio_get(gpio);
    if (gpio_cur == pressed) {
        // if not the same, cancel the timer and move on
        return 0;
    }

    // light up on board LED for visible feedback
    gpio_put(LED_PIN, pressed);

    // determine what to do based on which pin triggered
    // this can probably be cleaned up, but whatever it works for now
    switch (gpio) {
    case P1_UP: // udlr directions are 0-255, where 128 = "zero"
        if (pressed) {
            player1.y = 0;
        } else {
            player1.y = 128;
        }
        break;
    case P1_DOWN:
        if (pressed) {
            player1.y = 255;
        } else {
            player1.y = 128;
        }
        break;
    case P1_LEFT:
        if (pressed) {
            player1.x = 0;
        } else {
            player1.x = 128;
        }
        break;
    case P1_RIGHT:
        if (pressed) {
            player1.x = 255;
        } else {
            player1.x = 128;
        }
        break;
    case P1_A:
        player1.a = pressed;
        break;
    case P1_B:
        player1.b = pressed;
        break;
    case P1_C:
        player1.c = pressed;
        break;
    case P1_D:
        player1.d = pressed;
        break;
    case P1_E:
        player1.e = pressed;
        break;
    case P1_F:
        player1.f = pressed;
        break;
    case P1_G:
        player1.g = pressed;
        break;
    case P1_H:
        player1.h = pressed;
        break;
    case P1_START:
        player1.start = pressed;
        break;


    case P2_UP:
        if (pressed) {
            player2.y = 0;
        } else {
            player2.y = 128;
        }
        break;
    case P2_DOWN:
        if (pressed) {
            player2.y = 255;
        } else {
            player2.y = 128;
        }
        break;
    case P2_LEFT:
        if (pressed) {
            player2.x = 0;
        } else {
            player2.x = 128;
        }
        break;
    case P2_RIGHT:
        if (pressed) {
            player2.x = 255;
        } else {
            player2.x = 128;
        }
        break;
    case P2_A:
        player2.a = pressed;
        break;
    case P2_B:
        player2.b = pressed;
        break;
    case P2_C:
        player2.c = pressed;
        break;
    case P2_D:
        player2.d = pressed;
        break;
    case P2_E:
        player2.e = pressed;
        break;
    case P2_F:
        player2.f = pressed;
        break;
    case P2_G:
        player2.g = pressed;
        break;
    case P2_H:
        player2.h = pressed;
        break;
    case P2_START:
        player2.start = pressed;
        break;
    }

    return 0;
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

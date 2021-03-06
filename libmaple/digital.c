/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2012 LeafLabs, LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/*
 * Arduino-compatible digital I/O implementation.
 */

// #include <unwired.h>

//#include <wirish/io.h>
#include <io.h>

#include <libmaple/gpio.h>
#include <libmaple/timer.h>

//#include <wirish/wirish_time.h>
#include <time.h>

//#include <wirish/boards.h>
#include <boards.h>

uint32 digitalRead(uint8 pin) {
    if (pin >= BOARD_NR_GPIO_PINS) {
        return 0;
    }

    return gpio_read_bit(PIN_MAP[pin].gpio_device, PIN_MAP[pin].gpio_bit) ?
        HIGH : LOW;
}

void digitalWrite(uint8 pin, uint8 val) {
    if (pin >= BOARD_NR_GPIO_PINS) {
        return;
    }

    gpio_write_bit(PIN_MAP[pin].gpio_device, PIN_MAP[pin].gpio_bit, val);
}

void togglePin(uint8 pin) {
    if (pin >= BOARD_NR_GPIO_PINS) {
        return;
    }

    gpio_toggle_bit(PIN_MAP[pin].gpio_device, PIN_MAP[pin].gpio_bit);
}

#define BUTTON_DEBOUNCE_DELAY 1

uint8 isButtonPressed(uint8 pin, uint32 pressedLevel) {
    if (digitalRead(pin) == pressedLevel) {
        delay(BUTTON_DEBOUNCE_DELAY);
        while (digitalRead(pin) == pressedLevel)
            ;
        return 1;
    }
    return 0;
}

uint8 waitForButtonPress(uint32 timeout) {
    uint32 start = millis();
    uint32 time;
    if (timeout == 0) {
        while (!isButtonPressed( BOARD_BUTTON_PIN, BOARD_BUTTON_PRESSED_LEVEL))
            ;
        return 1;
    }
    do {
        time = millis();
        /* properly handle wrap-around */
        if ((start > time && time + (0xffffffffU - start) > timeout) ||
            time - start > timeout) {
            return 0;
        }
    } while (!isButtonPressed( BOARD_BUTTON_PIN, BOARD_BUTTON_PRESSED_LEVEL));
    // } while (!isButtonPressed());
    return 1;
}

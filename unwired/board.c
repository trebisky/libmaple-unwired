/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2011 LeafLabs, LLC.
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

/**
 * @file   wirish/boards/maple_mini/board.cpp
 * @author Marti Bolivar <mbolivar@leaflabs.com>
 * @brief  Maple Mini board file.
 */

/* Notes by tjt  11-9-2020
 *
 * The original Leaf Labs scheme had a boards directory inside of
 * the wirish directory.
 * Inside of that was a directory per board (maple, maple-mini, ...)
 * Inside of that (say "maple") was board.cpp
 * Also alongside that was include/board/board.h
 * So for each board we had 2 files via these paths:
 *  wirish/maple/board.cpp
 *  wirish/maple/include/board/board.h
 * Various source files in both wirish and libmaple would
 *   refer to "board/board.h"
 *
 * Now, I hate all of this chasing through twisty passages, all alike,
 *  looking for include files, so what I have done is this.
 *
 * Source files now refer simply to board.h as needed
 * All of the board.cpp files are now condensed into this file
 *  which is long and has various sections.
 *
 * This means each board.h file must define something like
 *  BOARD_MAPLE
 *  BOARD_MAPLE_MINI
 *  BOARD_BLUE_PILL
 * so that this file has something to take notice of.
 */

#include "board.h"

#include <libmaple/gpio.h>
#include <libmaple/timer.h>

//#include <wirish/wirish_debug.h>
#include "debug.h"

//#include <wirish/wirish_types.h>
#include "types.h"

/* Since we want the Serial Wire/JTAG pins as GPIOs, disable both SW
 * and JTAG debug support, unless configured otherwise. */

// tjt says NO, we never want this as it screws up ST-link downloads
// So I entirely override the CONFIG and take matters into my own hands
void boardInit(void) {
#ifndef CONFIG_MAPLE_MINI_NO_DISABLE_DEBUG
    // disableDebugPorts();
#endif
}

/* tjt -- The C compiler complains about GPIOB, TIMER3, ADC1, etc. etc.
 * being an initializer element and not constant.
 * This would be legal in C++
 */

// tjt - from wirish_types (now types.h)
// typedef struct stm32_pin_info {
//     gpio_dev *gpio_device;      /**< Maple pin's GPIO device
//     timer_dev *timer_device;    /**< Pin's timer device, if any
//     const adc_dev *adc_device;  /**< ADC device, if any
//     uint8 gpio_bit;             /**< Pin's GPIO port bit
//     uint8 timer_channel;        /**< Timer channel, or 0 if none
//     uint8 adc_channel;          /**< Pin ADC channel, or ADCx if none
// } stm32_pin_info;


#ifdef notdef
// tjt - not needed here, we get it from gpio.h
extern struct gpio_dev* const GPIOA;
extern struct gpio_dev* const GPIOB;
extern struct gpio_dev* const GPIOC;
#endif

#define X_GPIOA		((gpio_dev *) 1)
#define X_GPIOB		((gpio_dev *) 2)
#define X_GPIOC		((gpio_dev *) 3)

#define X_TIMER1	((timer_dev *) 1)
#define X_TIMER2	((timer_dev *) 2)
#define X_TIMER3	((timer_dev *) 3)
#define X_TIMER4	((timer_dev *) 4)

#define X_ADC1		((adc_dev *) 1)

//     timer_dev *timer_device;    /**< Pin's timer device, if any
//     const adc_dev *adc_device;  /**< ADC device, if any

/* ====================================================================== */
/* ====================================================================== */
/* ====================================================================== */

#ifdef BOARD_MAPLE

// Pin map: this lets the basic I/O functions (digitalWrite(),
// analogRead(), pwmWrite()) translate from pin numbers to STM32
// peripherals.
//
// PMAP_ROW() lets us specify a row (really a struct stm32_pin_info)
// in the pin map. Its arguments are:
//
// - GPIO device for the pin (GPIOA, etc.)
// - GPIO bit for the pin (0 through 15)
// - Timer device, or NULL if none
// - Timer channel (1 to 4, for PWM), or 0 if none
// - ADC device, or NULL if none
// - ADC channel, or ADCx if none

extern const stm32_pin_info PIN_MAP[BOARD_NR_GPIO_PINS] = {

    /* Top header */

    PMAP_ROW(GPIOA,   3, TIMER2,  4, ADC1,    3), /* D0/PA3 */
    PMAP_ROW(GPIOA,   2, TIMER2,  3, ADC1,    2), /* D1/PA2 */
    PMAP_ROW(GPIOA,   0, TIMER2,  1, ADC1,    0), /* D2/PA0 */
    PMAP_ROW(GPIOA,   1, TIMER2,  2, ADC1,    1), /* D3/PA1 */
    PMAP_ROW(GPIOB,   5,   NULL,  0, NULL, ADCx), /* D4/PB5 */
    PMAP_ROW(GPIOB,   6, TIMER4,  1, NULL, ADCx), /* D5/PB6 */
    PMAP_ROW(GPIOA,   8, TIMER1,  1, NULL, ADCx), /* D6/PA8 */
    PMAP_ROW(GPIOA,   9, TIMER1,  2, NULL, ADCx), /* D7/PA9 */
    PMAP_ROW(GPIOA,  10, TIMER1,  3, NULL, ADCx), /* D8/PA10 */
    PMAP_ROW(GPIOB,   7, TIMER4,  2, NULL, ADCx), /* D9/PB7 */
    PMAP_ROW(GPIOA,   4,   NULL,  0, ADC1,    4), /* D10/PA4 */
    PMAP_ROW(GPIOA,   7, TIMER3,  2, ADC1,    7), /* D11/PA7 */
    PMAP_ROW(GPIOA,   6, TIMER3,  1, ADC1,    6), /* D12/PA6 */
    PMAP_ROW(GPIOA,   5,   NULL,  0, ADC1,    5), /* D13/PA5 (LED) */
    PMAP_ROW(GPIOB,   8, TIMER4,  3, NULL, ADCx), /* D14/PB8 */

    /* Little header */

    PMAP_ROW(GPIOC,   0,   NULL,  0, ADC1,   10), /* D15/PC0 */
    PMAP_ROW(GPIOC,   1,   NULL,  0, ADC1,   11), /* D16/PC1 */
    PMAP_ROW(GPIOC,   2,   NULL,  0, ADC1,   12), /* D17/PC2 */
    PMAP_ROW(GPIOC,   3,   NULL,  0, ADC1,   13), /* D18/PC3 */
    PMAP_ROW(GPIOC,   4,   NULL,  0, ADC1,   14), /* D19/PC4 */
    PMAP_ROW(GPIOC,   5,   NULL,  0, ADC1,   15), /* D20/PC5 */

    /* External header */

    PMAP_ROW(GPIOC,  13,   NULL,  0, NULL, ADCx), /* D21/PC13 */
    PMAP_ROW(GPIOC,  14,   NULL,  0, NULL, ADCx), /* D22/PC14 */
    PMAP_ROW(GPIOC,  15,   NULL,  0, NULL, ADCx), /* D23/PC15 */
    PMAP_ROW(GPIOB,   9, TIMER4,  4, NULL, ADCx), /* D24/PB9 */
    PMAP_ROW(GPIOD,   2,   NULL,  0, NULL, ADCx), /* D25/PD2 */
    PMAP_ROW(GPIOC,  10,   NULL,  0, NULL, ADCx), /* D26/PC10 */
    PMAP_ROW(GPIOB,   0, TIMER3,  3, ADC1,    8), /* D27/PB0 */
    PMAP_ROW(GPIOB,   1, TIMER3,  4, ADC1,    9), /* D28/PB1 */
    PMAP_ROW(GPIOB,  10,   NULL,  0, NULL, ADCx), /* D29/PB10 */
    PMAP_ROW(GPIOB,  11,   NULL,  0, NULL, ADCx), /* D30/PB11 */
    PMAP_ROW(GPIOB,  12,   NULL,  0, NULL, ADCx), /* D31/PB12 */
    PMAP_ROW(GPIOB,  13,   NULL,  0, NULL, ADCx), /* D32/PB13 */
    PMAP_ROW(GPIOB,  14,   NULL,  0, NULL, ADCx), /* D33/PB14 */
    PMAP_ROW(GPIOB,  15,   NULL,  0, NULL, ADCx), /* D34/PB15 */
    PMAP_ROW(GPIOC,   6,   NULL,  0, NULL, ADCx), /* D35/PC6 */
    PMAP_ROW(GPIOC,   7,   NULL,  0, NULL, ADCx), /* D36/PC7 */
    PMAP_ROW(GPIOC,   8,   NULL,  0, NULL, ADCx), /* D37/PC8 */
    PMAP_ROW(GPIOC,   9,   NULL,  0, NULL, ADCx), /* D38/PC9 (BUT) */

    /* JTAG header */

    PMAP_ROW(GPIOA,  13,   NULL,  0, NULL, ADCx), /* D39/PA13 */
    PMAP_ROW(GPIOA,  14,   NULL,  0, NULL, ADCx), /* D40/PA14 */
    PMAP_ROW(GPIOA,  15,   NULL,  0, NULL, ADCx), /* D41/PA15 */
    PMAP_ROW(GPIOB,   3,   NULL,  0, NULL, ADCx), /* D42/PB3  */
    PMAP_ROW(GPIOB,   4,   NULL,  0, NULL, ADCx), /* D43/PB4  */
};

// Array of pins you can use for pwmWrite(). Keep it in Flash because
// it doesn't change, and so we don't waste RAM.
extern const uint8 boardPWMPins[] __FLASH__ = {
    0, 1, 2, 3, 5, 6, 7, 8, 9, 11, 12, 14, 24, 27, 28
};

// Array of pins you can use for analogRead().
extern const uint8 boardADCPins[] __FLASH__ = {
    0, 1, 2, 3, 10, 11, 12, 15, 16, 17, 18, 19, 20, 27, 28
};

// Array of pins that the board uses for something special. Other than
// the button and the LED, it's usually best to leave these pins alone
// unless you know what you're doing.
extern const uint8 boardUsedPins[] __FLASH__ = {
    BOARD_LED_PIN, BOARD_BUTTON_PIN, BOARD_JTMS_SWDIO_PIN,
    BOARD_JTCK_SWCLK_PIN, BOARD_JTDI_PIN, BOARD_JTDO_PIN, BOARD_NJTRST_PIN
};
#endif /* BOARD_MAPLE */

/* ====================================================================== */
/* ====================================================================== */
/* ====================================================================== */

#ifdef BOARD_MAPLE_MINI
stm32_pin_info PIN_MAP[BOARD_NR_GPIO_PINS] = {

    /* Top header */

    {X_GPIOB,   NULL, NULL, 11, 0, ADCx}, /* D0/PB11 */
    {X_GPIOB,   NULL, NULL, 10, 0, ADCx}, /* D1/PB10 */
    {X_GPIOB,   NULL, NULL,  2, 0, ADCx}, /* D2/PB2 */
    {X_GPIOB, X_TIMER3, X_ADC1,  0, 3,    8}, /* D3/PB0 */
    {X_GPIOA, X_TIMER3, X_ADC1,  7, 2,    7}, /* D4/PA7 */
    {X_GPIOA, X_TIMER3, X_ADC1,  6, 1,    6}, /* D5/PA6 */
    {X_GPIOA,   NULL, X_ADC1,  5, 0,    5}, /* D6/PA5 */
    {X_GPIOA,   NULL, X_ADC1,  4, 0,    4}, /* D7/PA4 */
    {X_GPIOA, X_TIMER2, X_ADC1,  3, 4,    3}, /* D8/PA3 */
    {X_GPIOA, X_TIMER2, X_ADC1,  2, 3,    2}, /* D9/PA2 */
    {X_GPIOA, X_TIMER2, X_ADC1,  1, 2,    1}, /* D10/PA1 */
    {X_GPIOA, X_TIMER2, X_ADC1,  0, 1,    0}, /* D11/PA0 */
    {X_GPIOC,   NULL, NULL, 15, 0, ADCx}, /* D12/PC15 */
    {X_GPIOC,   NULL, NULL, 14, 0, ADCx}, /* D13/PC14 */
    {X_GPIOC,   NULL, NULL, 13, 0, ADCx}, /* D14/PC13 */

    /* Bottom header */

    {X_GPIOB, X_TIMER4, NULL,  7, 2, ADCx}, /* D15/PB7 */
    {X_GPIOB, X_TIMER4, NULL,  6, 1, ADCx}, /* D16/PB6 */
    {X_GPIOB,   NULL, NULL,  5, 0, ADCx}, /* D17/PB5 */
    {X_GPIOB,   NULL, NULL,  4, 0, ADCx}, /* D18/PB4 */
    {X_GPIOB,   NULL, NULL,  3, 0, ADCx}, /* D19/PB3 */
    {X_GPIOA,   NULL, NULL, 15, 0, ADCx}, /* D20/PA15 */
    {X_GPIOA,   NULL, NULL, 14, 0, ADCx}, /* D21/PA14 */
    {X_GPIOA,   NULL, NULL, 13, 0, ADCx}, /* D22/PA13 */
    {X_GPIOA,   NULL, NULL, 12, 0, ADCx}, /* D23/PA12 */
    {X_GPIOA, X_TIMER1, NULL, 11, 4, ADCx}, /* D24/PA11 */
    {X_GPIOA, X_TIMER1, NULL, 10, 3, ADCx}, /* D25/PA10 */
    {X_GPIOA, X_TIMER1, NULL,  9, 2, ADCx}, /* D26/PA9 */
    {X_GPIOA, X_TIMER1, NULL,  8, 1, ADCx}, /* D27/PA8 */
    {X_GPIOB,   NULL, NULL, 15, 0, ADCx}, /* D28/PB15 */
    {X_GPIOB,   NULL, NULL, 14, 0, ADCx}, /* D29/PB14 */
    {X_GPIOB,   NULL, NULL, 13, 0, ADCx}, /* D30/PB13 */
    {X_GPIOB,   NULL, NULL, 12, 0, ADCx}, /* D31/PB12 */
    {X_GPIOB, X_TIMER4, NULL,  8, 3, ADCx}, /* D32/PB8 */
    {X_GPIOB, X_TIMER3, X_ADC1,  1, 4,    9}, /* D33/PB1 */
};

// tjt adds the following to do the initialization
//  that c++ would do.
void
initialize_pin_map ( void )
{
	int i;
	stm32_pin_info *pp;

	for ( i=0; i< BOARD_NR_GPIO_PINS; i++ ) {
	    pp = &PIN_MAP[i];
	    if ( pp->gpio_device == X_GPIOA )
		pp->gpio_device = GPIOA;
	    if ( pp->gpio_device == X_GPIOB )
		pp->gpio_device = GPIOB;
	    if ( pp->gpio_device == X_GPIOC )
		pp->gpio_device = GPIOC;

	    if ( pp->timer_device == X_TIMER1 )
		pp->timer_device = TIMER1;
	    if ( pp->timer_device == X_TIMER2 )
		pp->timer_device = TIMER2;
	    if ( pp->timer_device == X_TIMER3 )
		pp->timer_device = TIMER3;
	    if ( pp->timer_device == X_TIMER4 )
		pp->timer_device = TIMER4;

	    if ( pp->adc_device == X_ADC1 )
		pp->adc_device = ADC1;
	}
}

// extern const uint8 boardPWMPins[BOARD_NR_PWM_PINS] __FLASH__ = {
const uint8 boardPWMPins[BOARD_NR_PWM_PINS] __FLASH__ = {
    3, 4, 5, 8, 9, 10, 11, 15, 16, 25, 26, 27
};

// extern const uint8 boardADCPins[BOARD_NR_ADC_PINS] __FLASH__ = {
const uint8 boardADCPins[BOARD_NR_ADC_PINS] __FLASH__ = {
    3, 4, 5, 6, 7, 8, 9, 10, 11
};

#define USB_DP 23
#define USB_DM 24

// extern const uint8 boardUsedPins[BOARD_NR_USED_PINS] __FLASH__ = {
const uint8 boardUsedPins[BOARD_NR_USED_PINS] __FLASH__ = {
    BOARD_LED_PIN, BOARD_BUTTON_PIN, USB_DP, USB_DM
};
#endif /* BOARD_MAPLE_MINI */

/* ====================================================================== */
/* ====================================================================== */
/* ====================================================================== */

#ifdef ORIGINAL_MAPLE_MINI
// tjt - this business of declaring things extern is
//  peculiar.  Maybe this is a C++ thing.
// extern const stm32_pin_info PIN_MAP[BOARD_NR_GPIO_PINS] = {
// const stm32_pin_info PIN_MAP[BOARD_NR_GPIO_PINS] = {
stm32_pin_info PIN_MAP[BOARD_NR_GPIO_PINS] = {

    /* Top header */

    {GPIOB,   NULL, NULL, 11, 0, ADCx}, /* D0/PB11 */
    {GPIOB,   NULL, NULL, 10, 0, ADCx}, /* D1/PB10 */
    {GPIOB,   NULL, NULL,  2, 0, ADCx}, /* D2/PB2 */
    {GPIOB, TIMER3, ADC1,  0, 3,    8}, /* D3/PB0 */
    {GPIOA, TIMER3, ADC1,  7, 2,    7}, /* D4/PA7 */
    {GPIOA, TIMER3, ADC1,  6, 1,    6}, /* D5/PA6 */
    {GPIOA,   NULL, ADC1,  5, 0,    5}, /* D6/PA5 */
    {GPIOA,   NULL, ADC1,  4, 0,    4}, /* D7/PA4 */
    {GPIOA, TIMER2, ADC1,  3, 4,    3}, /* D8/PA3 */
    {GPIOA, TIMER2, ADC1,  2, 3,    2}, /* D9/PA2 */
    {GPIOA, TIMER2, ADC1,  1, 2,    1}, /* D10/PA1 */
    {GPIOA, TIMER2, ADC1,  0, 1,    0}, /* D11/PA0 */
    {GPIOC,   NULL, NULL, 15, 0, ADCx}, /* D12/PC15 */
    {GPIOC,   NULL, NULL, 14, 0, ADCx}, /* D13/PC14 */
    {GPIOC,   NULL, NULL, 13, 0, ADCx}, /* D14/PC13 */

    /* Bottom header */

    {GPIOB, TIMER4, NULL,  7, 2, ADCx}, /* D15/PB7 */
    {GPIOB, TIMER4, NULL,  6, 1, ADCx}, /* D16/PB6 */
    {GPIOB,   NULL, NULL,  5, 0, ADCx}, /* D17/PB5 */
    {GPIOB,   NULL, NULL,  4, 0, ADCx}, /* D18/PB4 */
    {GPIOB,   NULL, NULL,  3, 0, ADCx}, /* D19/PB3 */
    {GPIOA,   NULL, NULL, 15, 0, ADCx}, /* D20/PA15 */
    {GPIOA,   NULL, NULL, 14, 0, ADCx}, /* D21/PA14 */
    {GPIOA,   NULL, NULL, 13, 0, ADCx}, /* D22/PA13 */
    {GPIOA,   NULL, NULL, 12, 0, ADCx}, /* D23/PA12 */
    {GPIOA, TIMER1, NULL, 11, 4, ADCx}, /* D24/PA11 */
    {GPIOA, TIMER1, NULL, 10, 3, ADCx}, /* D25/PA10 */
    {GPIOA, TIMER1, NULL,  9, 2, ADCx}, /* D26/PA9 */
    {GPIOA, TIMER1, NULL,  8, 1, ADCx}, /* D27/PA8 */
    {GPIOB,   NULL, NULL, 15, 0, ADCx}, /* D28/PB15 */
    {GPIOB,   NULL, NULL, 14, 0, ADCx}, /* D29/PB14 */
    {GPIOB,   NULL, NULL, 13, 0, ADCx}, /* D30/PB13 */
    {GPIOB,   NULL, NULL, 12, 0, ADCx}, /* D31/PB12 */
    {GPIOB, TIMER4, NULL,  8, 3, ADCx}, /* D32/PB8 */
    {GPIOB, TIMER3, ADC1,  1, 4,    9}, /* D33/PB1 */
};
#endif

/* THE END */

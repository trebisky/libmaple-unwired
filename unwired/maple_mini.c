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

// #include <board/board.h>
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

#ifdef ORIGINAL
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

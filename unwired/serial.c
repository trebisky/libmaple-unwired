/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2011, 2012 LeafLabs, LLC.
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
 * @file wirish/HardwareSerial.cpp
 * @brief Wirish serial port implementation.
 */

// #include <wirish/HardwareSerial.h>

#include "boards.h"
#include "types.h"
#include "serial.h"
#include "board.h"

#include <libmaple/libmaple.h>
#include <libmaple/gpio.h>
#include <libmaple/timer.h>
#include <libmaple/usart.h>

// pins are defined in wirish/boards/maple_mini/include/board/board.h

/* The STM32F1 has 3 uarts (1,2,3) and USB */
#define NUM_SERIAL	4

enum serial_type{ HW_UART, USB_UART };

struct serial_info {
	enum serial_type type;	/* 0 = HW, 1 = USB */
	usart_dev *dev;
	int tx_pin;
	int rx_pin;
};

static struct serial_info serial_info[NUM_SERIAL];

/* F1 MCUs have no GPIO_AFR[HL], so turn off PWM if there's a conflict
 * on this GPIO bit. */
static void
disable_timer_if_necessary ( timer_dev *dev, uint8 ch)
{
    if (dev != NULL) {
        timer_set_mode(dev, ch, TIMER_DISABLED);
    }
}

int
serial_begin ( int port, int baud )
{
	int fd;
	struct serial_info *si;
	stm32_pin_info *txi;
	stm32_pin_info *rxi;

	if ( port < 1 || port >= NUM_SERIAL )
	    return -1;
	fd = port - 1;
	si = &serial_info[fd];
	si->type = HW_UART;

	if ( port == 1 ) {
	    si->dev = USART1;
	    si->tx_pin = BOARD_USART1_TX_PIN;
	    si->rx_pin = BOARD_USART1_RX_PIN;
	}
	if ( port == 2 ) {
	    si->dev = USART2;
	    si->tx_pin = BOARD_USART2_TX_PIN;
	    si->rx_pin = BOARD_USART2_RX_PIN;
	}
	if ( port == 3 ) {
	    si->dev = USART3;
	    si->tx_pin = BOARD_USART3_TX_PIN;
	    si->rx_pin = BOARD_USART3_RX_PIN;
	}

	ASSERT(baud <= si->dev->max_baud);

	if (baud > si->dev->max_baud) {
	    return -1;
	}

	txi = &PIN_MAP[si->tx_pin];
	rxi = &PIN_MAP[si->rx_pin];

	disable_timer_if_necessary ( txi->timer_device, txi->timer_channel );

	usart_config_gpios_async(si->dev,
                             rxi->gpio_device, rxi->gpio_bit,
                             txi->gpio_device, txi->gpio_bit,
                             0);

	usart_init ( si->dev );
	usart_set_baud_rate ( si->dev, USART_USE_PCLK, baud );
	usart_enable ( si->dev );

	return fd;
}

void
serial_write ( int fd, int ch )
{
	struct serial_info *si;

	si = &serial_info[fd];
	usart_putc ( si->dev, ch );
}

void
serial_putc ( int fd, int ch )
{
	serial_write ( fd, ch );
	if ( ch == '\n' )
	    serial_write ( fd, '\r' );
}


#ifdef OLD_CPP_SERIAL
#define DEFINE_HWSERIAL(name, n)                                   \
    HardwareSerial name(USART##n,                                  \
                        BOARD_USART##n##_TX_PIN,                   \
                        BOARD_USART##n##_RX_PIN)

#if BOARD_HAVE_USART1
DEFINE_HWSERIAL(Serial1, 1);
#endif
#if BOARD_HAVE_USART2
DEFINE_HWSERIAL(Serial2, 2);
#endif
#if BOARD_HAVE_USART3
DEFINE_HWSERIAL(Serial3, 3);
#endif
#if BOARD_HAVE_UART4
DEFINE_HWSERIAL(Serial4, 4);
#endif
#if BOARD_HAVE_UART5
DEFINE_HWSERIAL(Serial5, 5);
#endif
#if BOARD_HAVE_USART6
DEFINE_HWSERIAL(Serial6, 6);
#endif

HardwareSerial::HardwareSerial(usart_dev *usart_device,
                               uint8 tx_pin,
                               uint8 rx_pin) {
    this->usart_device = usart_device;
    this->tx_pin = tx_pin;
    this->rx_pin = rx_pin;
}

/*
 * Set up/tear down
 */

#if STM32_MCU_SERIES == STM32_SERIES_F1
/* F1 MCUs have no GPIO_AFR[HL], so turn off PWM if there's a conflict
 * on this GPIO bit. */
static void disable_timer_if_necessary(timer_dev *dev, uint8 ch) {
    if (dev != NULL) {
        timer_set_mode(dev, ch, TIMER_DISABLED);
    }
}
#elif (STM32_MCU_SERIES == STM32_SERIES_F2) ||    \
      (STM32_MCU_SERIES == STM32_SERIES_F4)
#define disable_timer_if_necessary(dev, ch) ((void)0)
#else
#warning "Unsupported STM32 series; timer conflicts are possible"
#endif

void HardwareSerial::begin(uint32 baud) {
    ASSERT(baud <= this->usart_device->max_baud);

    if (baud > this->usart_device->max_baud) {
        return;
    }

    const stm32_pin_info *txi = &PIN_MAP[this->tx_pin];
    const stm32_pin_info *rxi = &PIN_MAP[this->rx_pin];

    disable_timer_if_necessary(txi->timer_device, txi->timer_channel);

    usart_config_gpios_async(this->usart_device,
                             rxi->gpio_device, rxi->gpio_bit,
                             txi->gpio_device, txi->gpio_bit,
                             0);
    usart_init(this->usart_device);
    usart_set_baud_rate(this->usart_device, USART_USE_PCLK, baud);
    usart_enable(this->usart_device);
}

void HardwareSerial::end(void) {
    usart_disable(this->usart_device);
}

/*
 * I/O
 */

uint8 HardwareSerial::read(void) {
    // Block until a byte becomes available, to save user confusion.
    while (!this->available())
        ;
    return usart_getc(this->usart_device);
}

uint32 HardwareSerial::available(void) {
    return usart_data_available(this->usart_device);
}

void HardwareSerial::write(unsigned char ch) {
    usart_putc(this->usart_device, ch);
}

void HardwareSerial::flush(void) {
    usart_reset_rx(this->usart_device);
}

#endif

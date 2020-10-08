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
#include "board.h"

#include "serial.h"

#include <libmaple/libmaple.h>
#include <libmaple/gpio.h>
#include <libmaple/timer.h>
#include <libmaple/usart.h>

void usb_serial_begin ( void );
char usb_serial_getc ( void );
void usb_serial_putc ( char );
int usb_serial_available ( void );

// pins are defined in wirish/boards/maple_mini/include/board/board.h

/* The STM32F1 has 3 uarts (1,2,3) and USB */
#define NUM_SERIAL	4

enum serial_type{ HW_UART, USB_UART };

/* slot 0 is for USB
 * 1,2,3 are for HW
 */
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

	if ( port < 0 || port >= NUM_SERIAL )
	    return -1;

	/* It used to be different */
	fd = port;

	si = &serial_info[fd];
	si->type = HW_UART;

	if ( port == SERIAL_1 ) {
	    si->dev = USART1;
	    si->tx_pin = BOARD_USART1_TX_PIN;
	    si->rx_pin = BOARD_USART1_RX_PIN;
	}
	if ( port == SERIAL_2 ) {
	    si->dev = USART2;
	    si->tx_pin = BOARD_USART2_TX_PIN;
	    si->rx_pin = BOARD_USART2_RX_PIN;
	}
	if ( port == SERIAL_3 ) {
	    si->dev = USART3;
	    si->tx_pin = BOARD_USART3_TX_PIN;
	    si->rx_pin = BOARD_USART3_RX_PIN;
	}

	/* ignores baud rate */
	if ( port == SERIAL_USB ) {
	    si->type = USB_UART;
	    // We do this on boot up.
	    // usb_serial_begin ();
	    return fd;
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
	if ( serial_info[fd].type == USB_UART )
	    usb_serial_putc ( ch );
	else
	    usart_putc ( serial_info[fd].dev, ch );
}

void
serial_putc ( int fd, int ch )
{
	serial_write ( fd, ch );
	if ( ch == '\n' )
	    serial_write ( fd, '\r' );
}

/* This does not block */
int
serial_available ( int fd )
{
	if ( serial_info[fd].type == USB_UART )
	    return usb_serial_available ();
	else
	    return usart_data_available ( serial_info[fd].dev );
}

void
serial_flush ( int fd )
{
	if ( serial_info[fd].type == HW_UART )
	    usart_reset_rx ( serial_info[fd].dev );
}


uint8
serial_getc ( int fd )
{
	int rv;

	if ( serial_info[fd].type == USB_UART ) {
	    rv = usb_serial_getc ();
	} else {
	    // avoid confusion
	    while ( ! serial_available ( fd ) )
		;
	    rv = usart_getc ( serial_info[fd].dev );
	}

	if ( rv == '\r' )
	    rv = '\n';
	return rv;
}

/* synonym */
uint8
serial_read ( int fd )
{

	return serial_getc ( fd );
}

/* The following used to be in print.c, but that entire file
 * has been copied into here and gotten rid of.
 */
void
serial_puts ( int fd, char *str )
{
    while (*str)
        serial_putc ( fd, *str++ );
}

/* The usual base 10 case of what follows */
void
serial_print_num ( int fd, int n )
{
    int digit;

    if (n == 0) {
        serial_putc ( fd, '0' );
        return;
    }

    digit = n % 10;
    n /= 10;
    if ( n )
	serial_print_num ( fd, n );

    serial_putc ( fd, '0' + digit );
}

/* My hacked version of what is below.
 * note that it does NOT handle negatives.
 * Also, I use recursion in lieu of a buffer.
 */
void
serial_print_num_base ( int fd, int n, uint8 base)
{
    // unsigned char buf[CHAR_BIT * sizeof(long long)];
    // int i = 0;
    int digit;

    if (n == 0) {
        serial_putc ( fd, '0' );
        return;
    }

    digit = n % base;
    n /= base;
    if ( n )
	serial_print_num_base ( fd, n, base );

    serial_putc ( fd, (char) ( digit < 10 ?
                     '0' + digit :
                     'A' + digit - 10));

#ifdef notdef
    while (n > 0) {
        buf[i++] = n % base;
        n /= base;
    }

    for (; i > 0; i--) {
        print((char)(buf[i - 1] < 10 ?
                     '0' + buf[i - 1] :
                     'A' + buf[i - 1] - 10));
    }
#endif
}



/* -------------------------------------------------- */
/* -------------------------------------------------- */
/* -------------------------------------------------- */

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

/* -------------------------------------------------- */
/* -------------------------------------------------- */
/* -------------------------------------------------- */

/*
 * Print.cpp - Base class that provides print() and println()
 * Copyright (c) 2008 David A. Mellis.  All right reserved.
 * Copyright (c) 2011 LeafLabs, LLC.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * Modified 23 November 2006 by David A. Mellis
 * Modified 12 April 2011 by Marti Bolivar <mbolivar@leaflabs.com>
 */

/* The CPP print stuff is pretty C++ intensive, lots of polymorphism,
 * so I am not going crazy trying to replicate versions of it all
 */
#ifdef OLD_CPP_PRINT

#ifndef LLONG_MAX
/*
 * Note:
 *
 * At time of writing (12 April 2011), the limits.h that came with the
 * newlib we distributed didn't include LLONG_MAX.  Because we're
 * staying away from using templates (see /notes/coding_standard.rst,
 * "Language Features and Compiler Extensions"), this value was
 * copy-pasted from a println() of the value
 *
 *     std::numeric_limits<long long>::max().
 */
#define LLONG_MAX 9223372036854775807LL
#endif

/*
 * Public methods
 */

void Print::write(const char *str) {
    while (*str) {
        write(*str++);
    }
}

void Print::write(const void *buffer, uint32 size) {
    uint8 *ch = (uint8*)buffer;
    while (size--) {
        write(*ch++);
    }
}

void Print::print(uint8 b, int base) {
    print((uint64)b, base);
}

void Print::print(char c) {
    write(c);
}

void Print::print(const char str[]) {
    write(str);
}

void Print::print(int n, int base) {
    print((long long)n, base);
}

void Print::print(unsigned int n, int base) {
    print((unsigned long long)n, base);
}

void Print::print(long n, int base) {
    print((long long)n, base);
}

void Print::print(unsigned long n, int base) {
    print((unsigned long long)n, base);
}

void Print::print(long long n, int base) {
    if (base == BYTE) {
        write((uint8)n);
        return;
    }
    if (n < 0) {
        print('-');
        n = -n;
    }
    printNumber(n, base);
}

void Print::print(unsigned long long n, int base) {
    if (base == BYTE) {
        write((uint8)n);
    } else {
        printNumber(n, base);
    }
}

void Print::print(double n, int digits) {
    printFloat(n, digits);
}

void Print::println(void) {
    print('\r');
    print('\n');
}

void Print::println(char c) {
    print(c);
    println();
}

void Print::println(const char c[]) {
    print(c);
    println();
}

void Print::println(uint8 b, int base) {
    print(b, base);
    println();
}

void Print::println(int n, int base) {
    print(n, base);
    println();
}

void Print::println(unsigned int n, int base) {
    print(n, base);
    println();
}

void Print::println(long n, int base) {
    print((long long)n, base);
    println();
}

void Print::println(unsigned long n, int base) {
    print((unsigned long long)n, base);
    println();
}

void Print::println(long long n, int base) {
    print(n, base);
    println();
}

void Print::println(unsigned long long n, int base) {
    print(n, base);
    println();
}

void Print::println(double n, int digits) {
    print(n, digits);
    println();
}

/*
 * Private methods
 */

void Print::printNumber(unsigned long long n, uint8 base) {
    unsigned char buf[CHAR_BIT * sizeof(long long)];
    unsigned long i = 0;

    if (n == 0) {
        print('0');
        return;
    }

    while (n > 0) {
        buf[i++] = n % base;
        n /= base;
    }

    for (; i > 0; i--) {
        print((char)(buf[i - 1] < 10 ?
                     '0' + buf[i - 1] :
                     'A' + buf[i - 1] - 10));
    }
}

/* According to snprintf(),
 *
 * nextafter((double)numeric_limits<long long>::max(), 0.0) ~= 9.22337e+18
 *
 * This slightly smaller value was picked semi-arbitrarily. */
#define LARGE_DOUBLE_TRESHOLD (9.1e18)

/* THIS FUNCTION SHOULDN'T BE USED IF YOU NEED ACCURATE RESULTS.
 *
 * This implementation is meant to be simple and not occupy too much
 * code size.  However, printing floating point values accurately is a
 * subtle task, best left to a well-tested library function.
 *
 * See Steele and White 2003 for more details:
 *
 * http://kurtstephens.com/files/p372-steele.pdf
 */
void Print::printFloat(double number, uint8 digits) {
    // Hackish fail-fast behavior for large-magnitude doubles
    if (abs(number) >= LARGE_DOUBLE_TRESHOLD) {
        if (number < 0.0) {
            print('-');
        }
        print("<large double>");
        return;
    }

    // Handle negative numbers
    if (number < 0.0) {
        print('-');
        number = -number;
    }

    // Simplistic rounding strategy so that e.g. print(1.999, 2)
    // prints as "2.00"
    double rounding = 0.5;
    for (uint8 i = 0; i < digits; i++) {
        rounding /= 10.0;
    }
    number += rounding;

    // Extract the integer part of the number and print it
    long long int_part = (long long)number;
    double remainder = number - int_part;
    print(int_part);

    // Print the decimal point, but only if there are digits beyond
    if (digits > 0) {
        print(".");
    }

    // Extract digits from the remainder one at a time
    while (digits-- > 0) {
        remainder *= 10.0;
        int to_print = (int)remainder;
        print(to_print);
        remainder -= to_print;
    }
}
#endif

/* THE END */

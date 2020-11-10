/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
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
 * @brief USB virtual serial terminal
 */

// #include <wirish/usb_serial.h>

#include <string.h>
// #include <stdint.h>

#include <libmaple/nvic.h>
#include <usb/usb_cdcacm.h>
#include <usb/usb.h>
// #include <libmaple/iwdg.h>

// #include <wirish/wirish.h>
// #include "unwired.h"

#include <libmaple/systick.h>
#include "time.h"
#include "serial.h"

#ifndef BOARD_BLUE_PILL
static void rxHook(unsigned, void*);
static void ifaceSetupHook(unsigned, void*);
#endif

/* Moved here from usb/usb_cdcacm.c */
static void
usb_serial_disconnect ( gpio_dev *disc_dev, uint8 disc_bit )
{
    /* Present ourselves to the host. Writing 0 to "disc" pin must
     * pull USB_DP pin up while leaving USB_DM pulled down by the
     * transceiver. See USB 2.0 spec, section 7.1.7.3. */
    gpio_set_mode(disc_dev, disc_bit, GPIO_OUTPUT_PP);
    gpio_write_bit(disc_dev, disc_bit, 0);
}

/* The routines in this file are never intended to be called directly,
 * so I don't provide public prototypes.
 * The intent is that they will be called from routines in serial.c
 * tjt
 */

/* True maple boards have a USB disconnect circuit
 * ( a couple of transistors) controlled by a GPIO pin.
 * This allows software to perform a USB disconnect whe
 * it starts up, causing the host system to take a fresh
 * look at the USB device.
 * The maple does this because when it is running the Maple
 * DFU boot loader, it runs the loader after reset for
 * couple of seconds, then when the application starts
 * (this code), it wants to act like an ACM serial device,
 * not a DFU loader.
 * The hooks and the DISC bit are all about this loader scheme.
 */

void
usb_serial_begin (void)
{
    // printf ( "USB usb_serial_begin\n" );
#ifndef BOARD_BLUE_PILL
    usb_serial_disconnect ( BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT );
#endif
    // usb_cdcacm_enable ( BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT );
    usb_cdcacm_enable ();

#ifndef BOARD_BLUE_PILL
    usb_cdcacm_set_hooks(USB_CDCACM_HOOK_RX, rxHook);
    usb_cdcacm_set_hooks(USB_CDCACM_HOOK_IFACE_SETUP, ifaceSetupHook);
#endif
}

void
usb_serial_end ( void )
{
    // usb_cdcacm_disable(BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT);
    usb_cdcacm_disable ();
#ifndef BOARD_BLUE_PILL
    gpio_write_bit ( BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT, 1 );
#endif
    usb_cdcacm_remove_hooks(USB_CDCACM_HOOK_RX | USB_CDCACM_HOOK_IFACE_SETUP);
}

void
usb_serial_wait ( void )
{
    usb_cdcacm_wait ();
}

/* With timeout for config */
/* Caller should also call usb_check() on return
 *  to verify whether the connection configured or not
 */
void
usb_serial_wait_t (int timeout)
{
    unsigned int start = systick_uptime ();

    for ( ;; ) {
	if ( usb_check () )
	    return;
	if ( systick_uptime() - start > timeout )
	    return;
    }
}

#define USB_TIMEOUT 50

// static void
void
usb_serial_write ( const char *buf, int len )
{
    uint32 txed = 0;
    uint32 old_txed = 0;
    uint32 start = millis();
    uint32 sent = 0;

    if ( ! usb_is_connected(USBLIB) )
	return;
    if ( ! usb_is_configured(USBLIB) )
	return;
    if ( ! buf )
        return;

    while ( txed < len && (millis() - start < USB_TIMEOUT) ) {
        sent = usb_cdcacm_tx((const uint8*)buf + txed, len - txed);
        txed += sent;
        if (old_txed != txed) {
            start = millis();
        }
        old_txed = txed;
    }


    if (sent == USB_CDCACM_TX_EPSIZE) {
        while (usb_cdcacm_is_transmitting() != 0) {
        }
        /* flush out to avoid having the pc wait for more data */
        usb_cdcacm_tx(NULL, 0);
    }
}

void
usb_serial_putc ( uint8 ch )
{
    usb_serial_write ( &ch, 1 );
}

void
usb_serial_puts ( const char *str )
{
    usb_serial_write ( str, strlen(str) );
}

int
usb_serial_available ( void )
{
    return usb_cdcacm_data_available();
}

/* blocks */
static int
usb_serial_readbuf ( void *buf, int len )
{
    int rxed = 0;

    if (!buf)
        return 0;

    while ( rxed < len ) {
        rxed += usb_cdcacm_rx ( (char *)buf + rxed, len - rxed);
    }

    return rxed;
}

/* Blocks forever until 1 byte is received */
int
usb_serial_getc ( void )
{
    char b;
    usb_serial_readbuf ( &b, 1 );
    return b;
}

/* ------------------------------------------------ */
/* ------------------------------------------------ */

/* This is all about the disconnect pin,
 * which does not exist on a Blue Pill
 */

#ifdef notdef
/* The maple mini had this pin wired to a USB disconnect circuit,
 * but the blue pill does no such thing.
 */
#define BOARD_USB_DISC_DEV        GPIOB
#define BOARD_USB_DISC_BIT        9

static void
usb_disc_enable(gpio_dev *disc_dev, uint8 disc_bit)
{
    /* Present ourselves to the host. Writing 0 to "disc" pin must
     * pull USB_DP pin up while leaving USB_DM pulled down by the
     * transceiver. See USB 2.0 spec, section 7.1.7.3. */
    gpio_set_mode(disc_dev, disc_bit, GPIO_OUTPUT_PP);
    gpio_write_bit(disc_dev, disc_bit, 0);
}

static void
usb_disc_disable(gpio_dev *disc_dev, uint8 disc_bit)
{
    /* Turn off the interrupt and signal disconnect (see e.g. USB 2.0
     * spec, section 7.1.7.3). */
    nvic_irq_disable(NVIC_USB_LP_CAN_RX0);
    gpio_write_bit(disc_dev, disc_bit, 1);
}
#endif

/*
 * Bootloader hook stuff
 */

#ifndef BOARD_BLUE_PILL
enum reset_state_t {
    DTR_UNSET,
    DTR_HIGH,
    DTR_NEGEDGE,
    DTR_LOW
};

static enum reset_state_t reset_state = DTR_UNSET;

static void ifaceSetupHook ( unsigned hook, void *requestvp )
{
    uint8 request = *(uint8*)requestvp;

    printf ( "USB ifaceSetupHook\n" );
    // Ignore requests we're not interested in.
    if (request != USB_CDCACM_SET_CONTROL_LINE_STATE) {
        return;
    }

/* defined on cc line via -D */
// #define BOOTLOADER_maple

#if defined(BOOTLOADER_maple)
    // We need to see a negative edge on DTR before we start looking
    // for the in-band magic reset byte sequence.
    uint8 dtr = usb_cdcacm_get_dtr();
    switch (reset_state) {
    case DTR_UNSET:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_HIGH:
        reset_state = dtr ? DTR_HIGH : DTR_NEGEDGE;
        break;
    case DTR_NEGEDGE:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_LOW:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    }
#endif

#if defined(BOOTLOADER_robotis)
    uint8 dtr = usb_cdcacm_get_dtr();
    uint8 rts = usb_cdcacm_get_rts();

    if (rts && !dtr) {
        reset_state = DTR_NEGEDGE;
    }
#endif
}

#define RESET_DELAY 100000

#if defined(BOOTLOADER_maple)
static void
wait_reset ( void )
{
  delay_us(RESET_DELAY);
  nvic_sys_reset();
}
#endif

#define STACK_TOP 0x20000800
#define EXC_RETURN 0xFFFFFFF9
#define DEFAULT_CPSR 0x61000000

static void
rxHook( unsigned hook, void *ignored )
{
    printf ( "USB rxHook\n" );
    /* FIXME this is mad buggy; we need a new reset sequence. E.g. NAK
     * after each RX means you can't reset if any bytes are waiting. */
    if (reset_state == DTR_NEGEDGE) {
        reset_state = DTR_LOW;

        if (usb_cdcacm_data_available() >= 4) {

#if defined(BOOTLOADER_maple)
            // The magic reset sequence is "1EAF".
            static const uint8 magic[4] = {'1', 'E', 'A', 'F'};
#endif

#if defined(BOOTLOADER_robotis)
            static const uint8 magic[4] = {'C', 'M', '9', 'X'};
#endif
            uint8 chkBuf[4];

            // Peek at the waiting bytes, looking for reset sequence,
            // bailing on mismatch.
            usb_cdcacm_peek(chkBuf, 4);
            for (unsigned i = 0; i < sizeof(magic); i++) {
                if (chkBuf[i] != magic[i]) {
                    return;
                }
            }

#if defined(BOOTLOADER_maple)
            // Got the magic sequence -> reset, presumably into the bootloader.
            // Return address is wait_reset, but we must set the thumb bit.
            int target = (int)wait_reset | 0x1;

            asm volatile("mov r0, %[stack_top]      \n\t" // Reset stack
                         "mov sp, r0                \n\t"
                         "mov r0, #1                \n\t"
                         "mov r1, %[target_addr]    \n\t"
                         "mov r2, %[cpsr]           \n\t"
                         "push {r2}                 \n\t" // Fake xPSR
                         "push {r1}                 \n\t" // PC target addr
                         "push {r0}                 \n\t" // Fake LR
                         "push {r0}                 \n\t" // Fake R12
                         "push {r0}                 \n\t" // Fake R3
                         "push {r0}                 \n\t" // Fake R2
                         "push {r0}                 \n\t" // Fake R1
                         "push {r0}                 \n\t" // Fake R0
                         "mov lr, %[exc_return]     \n\t"
                         "bx lr"
                         :
                         : [stack_top] "r" (STACK_TOP),
                           [target_addr] "r" (target),
                           [exc_return] "r" (EXC_RETURN),
                           [cpsr] "r" (DEFAULT_CPSR)
                         : "r0", "r1", "r2");
#endif

#if defined(BOOTLOADER_robotis)
            iwdg_init(IWDG_PRE_4, 10);
#endif

            /* Can't happen. */
            ASSERT_FAULT(0);
        }
    }
}
#endif /* BOOTLOADER_HOOK_RUBBISH */

/* ==================================================== */
/* ==================================================== */
/* ==================================================== */
/* ==================================================== */

#ifdef OLD_CPP_SERIALUSB

#define BOARD_HAVE_SERIALUSB (defined(BOARD_USB_DISC_DEV) && \
                              defined(BOARD_USB_DISC_BIT))

/* --- start old usb_serial.h --- */

#include <wirish/Print.h>
#include <wirish/boards.h>

/**
 * @brief Virtual serial terminal.
 */
class USBSerial : public Print {
public:
    USBSerial(void);

    void begin(void);
    void end(void);

    uint32 available(void);

    uint32 read(void *buf, uint32 len);
    uint8  read(void);

    void write(uint8);
    void write(const char *str);
    void write(const void*, uint32);

    uint8 getRTS();
    uint8 getDTR();
    uint8 isConnected();
    uint8 pending();
};

#if BOARD_HAVE_SERIALUSB
extern USBSerial SerialUSB;
#endif


/* --- start old usb_serial.cpp --- */
/*
 * Hooks used for bootloader reset signalling
 */

#if BOARD_HAVE_SERIALUSB
static void rxHook(unsigned, void*);
static void ifaceSetupHook(unsigned, void*);
#endif

/*
 * USBSerial interface
 */

USBSerial::USBSerial(void) {
#if !BOARD_HAVE_SERIALUSB
    ASSERT(0);
#endif
}

void USBSerial::begin(void) {
#if BOARD_HAVE_SERIALUSB
    usb_cdcacm_enable(BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT);
    usb_cdcacm_set_hooks(USB_CDCACM_HOOK_RX, rxHook);
    usb_cdcacm_set_hooks(USB_CDCACM_HOOK_IFACE_SETUP, ifaceSetupHook);
#endif
}

void USBSerial::end(void) {
#if BOARD_HAVE_SERIALUSB
    usb_cdcacm_disable(BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT);
    usb_cdcacm_remove_hooks(USB_CDCACM_HOOK_RX | USB_CDCACM_HOOK_IFACE_SETUP);
#endif
}

void USBSerial::write(uint8 ch) {
    this->write(&ch, 1);
}

void USBSerial::write(const char *str) {
    this->write(str, strlen(str));
}

#define USB_TIMEOUT 50

void USBSerial::write(const void *buf, uint32 len) {
    if (!this->isConnected() || !buf) {
        return;
    }

    uint32 txed = 0;
    uint32 old_txed = 0;
    uint32 start = millis();

    uint32 sent = 0;

    while (txed < len && (millis() - start < USB_TIMEOUT)) {
        sent = usb_cdcacm_tx((const uint8*)buf + txed, len - txed);
        txed += sent;
        if (old_txed != txed) {
            start = millis();
        }
        old_txed = txed;
    }


    if (sent == USB_CDCACM_TX_EPSIZE) {
        while (usb_cdcacm_is_transmitting() != 0) {
        }
        /* flush out to avoid having the pc wait for more data */
        usb_cdcacm_tx(NULL, 0);
    }
}

uint32 USBSerial::available(void) {
    return usb_cdcacm_data_available();
}

uint32 USBSerial::read(void *buf, uint32 len) {
    if (!buf) {
        return 0;
    }

    uint32 rxed = 0;
    while (rxed < len) {
        rxed += usb_cdcacm_rx((uint8*)buf + rxed, len - rxed);
    }

    return rxed;
}

/* Blocks forever until 1 byte is received */
uint8 USBSerial::read(void) {
    uint8 b;
    this->read(&b, 1);
    return b;
}

uint8 USBSerial::pending(void) {
    return usb_cdcacm_get_pending();
}

uint8 USBSerial::isConnected(void) {
    return usb_is_connected(USBLIB) && usb_is_configured(USBLIB);
}

uint8 USBSerial::getDTR(void) {
    return usb_cdcacm_get_dtr();
}

uint8 USBSerial::getRTS(void) {
    return usb_cdcacm_get_rts();
}

#if BOARD_HAVE_SERIALUSB
USBSerial SerialUSB;
#endif

/*
 * Bootloader hook implementations
 */

#if BOARD_HAVE_SERIALUSB

enum reset_state_t {
    DTR_UNSET,
    DTR_HIGH,
    DTR_NEGEDGE,
    DTR_LOW
};

static reset_state_t reset_state = DTR_UNSET;

static void ifaceSetupHook(unsigned hook, void *requestvp) {
    uint8 request = *(uint8*)requestvp;

    // Ignore requests we're not interested in.
    if (request != USB_CDCACM_SET_CONTROL_LINE_STATE) {
        return;
    }

#if defined(BOOTLOADER_maple)
    // We need to see a negative edge on DTR before we start looking
    // for the in-band magic reset byte sequence.
    uint8 dtr = usb_cdcacm_get_dtr();
    switch (reset_state) {
    case DTR_UNSET:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_HIGH:
        reset_state = dtr ? DTR_HIGH : DTR_NEGEDGE;
        break;
    case DTR_NEGEDGE:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_LOW:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    }
#endif

#if defined(BOOTLOADER_robotis)
    uint8 dtr = usb_cdcacm_get_dtr();
    uint8 rts = usb_cdcacm_get_rts();

    if (rts && !dtr) {
        reset_state = DTR_NEGEDGE;
    }
#endif
}

#define RESET_DELAY 100000
#if defined(BOOTLOADER_maple)
static void wait_reset(void) {
  delay_us(RESET_DELAY);
  nvic_sys_reset();
}
#endif

#define STACK_TOP 0x20000800
#define EXC_RETURN 0xFFFFFFF9
#define DEFAULT_CPSR 0x61000000
static void rxHook(unsigned hook, void *ignored) {
    /* FIXME this is mad buggy; we need a new reset sequence. E.g. NAK
     * after each RX means you can't reset if any bytes are waiting. */
    if (reset_state == DTR_NEGEDGE) {
        reset_state = DTR_LOW;

        if (usb_cdcacm_data_available() >= 4) {
            // The magic reset sequence is "1EAF".
#if defined(BOOTLOADER_maple)
            static const uint8 magic[4] = {'1', 'E', 'A', 'F'};
#endif
#if defined(BOOTLOADER_robotis)
            static const uint8 magic[4] = {'C', 'M', '9', 'X'};
#endif
            uint8 chkBuf[4];

            // Peek at the waiting bytes, looking for reset sequence,
            // bailing on mismatch.
            usb_cdcacm_peek(chkBuf, 4);
            for (unsigned i = 0; i < sizeof(magic); i++) {
                if (chkBuf[i] != magic[i]) {
                    return;
                }
            }

#if defined(BOOTLOADER_maple)
            // Got the magic sequence -> reset, presumably into the bootloader.
            // Return address is wait_reset, but we must set the thumb bit.
            int target = (int) wait_reset | 0x1;
            asm volatile("mov r0, %[stack_top]      \n\t" // Reset stack
                         "mov sp, r0                \n\t"
                         "mov r0, #1                \n\t"
                         "mov r1, %[target_addr]    \n\t"
                         "mov r2, %[cpsr]           \n\t"
                         "push {r2}                 \n\t" // Fake xPSR
                         "push {r1}                 \n\t" // PC target addr
                         "push {r0}                 \n\t" // Fake LR
                         "push {r0}                 \n\t" // Fake R12
                         "push {r0}                 \n\t" // Fake R3
                         "push {r0}                 \n\t" // Fake R2
                         "push {r0}                 \n\t" // Fake R1
                         "push {r0}                 \n\t" // Fake R0
                         "mov lr, %[exc_return]     \n\t"
                         "bx lr"
                         :
                         : [stack_top] "r" (STACK_TOP),
                           [target_addr] "r" (target),
                           [exc_return] "r" (EXC_RETURN),
                           [cpsr] "r" (DEFAULT_CPSR)
                         : "r0", "r1", "r2");
#endif

#if defined(BOOTLOADER_robotis)
            iwdg_init(IWDG_PRE_4, 10);
#endif

            /* Can't happen. */
            ASSERT_FAULT(0);
        }
    }
}

#endif  // BOARD_HAVE_SERIALUSB

#endif /* OLD_CPP_SERIALUSB */

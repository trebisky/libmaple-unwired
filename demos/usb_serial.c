// Tests USB serial
// usb_serial.c

// Tom Trebisky  10-13-2020
//
// This is finally a clean example of how to use serial USB.

// #include <unwired.h>

#include <serial.h>
#include <io.h>
#include <time.h>

/* 
 * The trick to making USB work was to do two things:
 *  a) go into a loop waiting for USB to get connected and configured.
 *  b) plug and unplug the USB cable
 *
 * The actual maple mini had some circuitry to do a USB disconnect.
 * by yanking on a certain GPIO pin that was wired to the circuit,
 * you could pull one of the USB lines low (or was it high) and signal
 * a disconnect.  With a blue pill, we don't have that and have to unplug
 *  and replug.  At least we do for development and testing.
 * If this was a finished gadget, we would plug it in and then it would
 *  set up USB and all would be well.  I think the wait loop would be
 *  required in any case.
 *
 * With the original Maple code, my linux system
 * will identify this as /dev/ttyACM0
 *  kernel: usb 1-1.1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
 *  kernel: usb 1-1.1.1: Product: Maple
 *  kernel: usb 1-1.1.1: Manufacturer: LeafLabs
 *  kernel: cdc_acm 1-1.1.1:1.0: ttyACM0: USB ACM device
 */

/* With this demo, the idea is that I have moved the usb_wait
 * into the unwired/serial.c library.
 */

/* Here is what we see (at least on Fedora 30 linux)
 *
 * First, get rid of ModemManager ifit is running on your system.
 *
 * 1 - We load code into the board and we see "unconnected" status forever.
 * 2 - We unplug the USB cable.  Linux sees a disconnect.
 *				this code still sees "unconnected"
 * 3 - We replug the cable.  Linux sees /dev/ttyACM0
 *				this code goes to "configured" !!
 * Now this code is running, but probably nothing is listening yet on
 * the linux side.  Any output will be lost, but we will not hang
 * writing into thin air.  For some applications this will be fine,
 * for example if we just spit out a telemetry stream that a linux side
 * application can start listening to at any point.
 *
 * What this code does though is to wait for a character to by typed
 * on USB, then begins spewing messages for ever and ever.
 * The character wait ensures that we see message 1.
 *
 * I use "picocom /dev/ttyACM0" on the linux side to talk to this.
 */

void
main(void)
{
    int fd_usb;
    int fds;
    int count;
    int c;

    pinMode(BOARD_LED_PIN, OUTPUT);

    fds = serial_begin ( SERIAL_1, 115200 );
    set_std_serial ( fds );

    printf ( "serial baud rate = %d\n", 115200 );


    printf ( "Waiting for USB to begin\n" );
    fd_usb = serial_begin ( SERIAL_USB, 999 );
    printf ( "-- USB Happy and Starting\n");

    printf ( "Waiting for input from USB\n" );

    c = serial_getc ( fd_usb );
    printf ( "Got input from USB: %h %c\n", c, c );

    for ( count = 1; ; count++ ) {

	toggleLED();

	delay ( 1000 );

        serial_printf ( fd_usb, "Count = %d\n", count );
    }
}

/* THE END */

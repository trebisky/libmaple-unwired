// Tests the SysTick enable/disable functions

#include <unwired.h>

#include <libmaple/systick.h>
#include <libmaple/usb_cdcacm.h>
#include <libmaple/usb.h>

// tjt -- we ain't got no button
// so we code something similar using a keystroke

/* Using this with USB was a real nightmare, and yielded some valuable lessons.
 * Out of suffering comes enlightenment?
 *
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
 */

int enable = 1;

int
usb_check_noisy ( void )
{
    int ok = 1;

    if ( usb_is_connected(USBLIB) )
        puts ( "USB is connected\n" );
    else {
        puts ( "USB is NOT connected\n" );
	ok = 0;
    }

    if ( usb_is_configured(USBLIB) )
        puts ( "USB is configured\n" );
    else {
        puts ( "USB is NOT configured\n" );
	ok = 0;
    }
    return ok;
}

int
usb_check ( void )
{
    int ok = 1;

    if ( ! usb_is_connected(USBLIB) )
	ok = 0;

    if ( ! usb_is_configured(USBLIB) )
	ok = 0;

    return ok;
}

void
usb_wait ( void )
{
    if ( usb_check() )
	return;

    printf ( "Waiting for USB\n" );

    for ( ;; ) {
	if ( usb_check () )
	    return;
	printf ( "..Wait\n" );
	delay ( 1000 );
    }
}

void
main(void)
{
    int fd;
    int fds;
    int c;
    int count;
    int i;

    pinMode(BOARD_LED_PIN, OUTPUT);

    // fd = serial_begin ( SERIAL_1, 115200 );
    fds = serial_begin ( SERIAL_1, 115200 );
    set_std_serial ( fds );

    printf ( "serial baud rate = %d\n", 115200 );

    // With the original Maple code, my linux system
    // will identify this as /dev/ttyACM0
    //  kernel: usb 1-1.1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
    //  kernel: usb 1-1.1.1: Product: Maple
    //  kernel: usb 1-1.1.1: Manufacturer: LeafLabs
    //  kernel: cdc_acm 1-1.1.1:1.0: ttyACM0: USB ACM device

    fd = serial_begin ( SERIAL_USB, 999 );
    usb_wait ();
    serial_puts ( fd, "-- USB Starting\n");

    printf ( "USB fd = %d\n", fd );

    for ( ;; ) {

	toggleLED();

#ifdef notdef
	// An artificial delay
	// This works, but it is really fast.
	for(i = 0; i < 150000; i++)
	    ;
#endif
	delay ( 1000 );

	count = millis ();
        serial_printf ( fd, "Systick count = %d\n", count );

	if ( ! serial_available ( fd ) )
	    continue;

        c = serial_getc(fd);

        if (enable) {
            systick_disable();
            serial_puts ( fd, "-- Disabling SysTick\n");
	    enable = 0;
        } else {
            serial_puts ( fd, "-- Re-enabling SysTick\n");
            systick_enable();
	    enable = 1;
        }

    }
}

/* THE END */

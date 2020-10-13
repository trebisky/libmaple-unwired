// Tests USB serial

#include <unwired.h>

#include <libmaple/systick.h>
#include <libmaple/usb_cdcacm.h>
#include <libmaple/usb.h>

/* Using this with USB was a real nightmare, and yielded some valuable lessons.
 * Out of suffering comes enlightenment?!!
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
 *
 * With the original Maple code, my linux system
 * will identify this as /dev/ttyACM0
 *  kernel: usb 1-1.1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
 *  kernel: usb 1-1.1.1: Product: Maple
 *  kernel: usb 1-1.1.1: Manufacturer: LeafLabs
 *  kernel: cdc_acm 1-1.1.1:1.0: ttyACM0: USB ACM device
 */

/* This hides the true state variable.
 * Once value can be UNCONNECTED
 * So CONNECTED means any other value,
 * including CONFIGURED.
 */
int
usb_check ( void )
{
    if ( usb_is_configured(USBLIB) )
	return 'C';

    if ( usb_is_connected(USBLIB) )
	return 'K';

    return 'U';
}

void
usb_wait ( void )
{
    int stat;

    if ( usb_check() == 'C' )
	return;

    printf ( "Waiting for USB\n" );

    for ( ;; ) {
	stat = usb_check ();
	if ( stat == 'U' )
	    printf ( "USB unconnected\n" );
	if ( stat == 'K' )
	    printf ( "USB connected\n" );
	if ( stat == 'C' ) {
	    printf ( "USB configured !!!\n" );
	    return;
	}
	printf ( "..Wait\n" );
	delay ( 1000 );
    }
}

/* Here is what we see.
 * 1 - We load code into the board and we see "unconnected" forever.
 * 2 - We unplug the USB cable.  Linux sees a disconnect.
 *				this code still sees "unconnected"
 * 3 - We replug the cable.  Linux sees /dev/ttyACM0
 *				this code goes to "configured"
 * So this code runs, but nothing is listening yet on the
 *  linux side.  In fact when I try to run picocom, it rejects
 *  it the first few times.  When it finally does allow me to
 *  connect, I have missed quite a bit of output.
 */

void
main(void)
{
    int fd;
    int fds;
    int count;

    pinMode(BOARD_LED_PIN, OUTPUT);

    fds = serial_begin ( SERIAL_1, 115200 );
    set_std_serial ( fds );

    printf ( "serial baud rate = %d\n", 115200 );

    fd = serial_begin ( SERIAL_USB, 999 );
    usb_wait ();
    printf ( "-- USB Happy and Starting\n");

    for ( count = 1; ; count++ ) {

	toggleLED();

	delay ( 1000 );

        serial_printf ( fd, "Count = %d\n", count );
    }
}

/* THE END */

// Tests USB serial
// usb_serial2.c
// Don't try to run this, move right along to usb_serial3
// This has experimentation that led me to discover that
// ModemManager on my fedora linux system was screwing up the
// USB interaction with this code.  Once I eradicated
// ModemManager, this became obsolete and I moved on to
// usb_serial3.c

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

void
usb_xxx ( void )
{
    for ( ;; ) {
	printf ( "USB stat: %d\n", usb_cdcacm_check() );
	delay ( 1000 );
    }
}

/* With this demo, the idea is that I have moved the usb_wait
 * into the unwired/serial.c library.
 */

/* Here is what we see.
 * 1 - We load code into the board and we see "unconnected" forever.
 * 2 - We unplug the USB cable.  Linux sees a disconnect.
 *				this code still sees "unconnected"
 * 3 - We replug the cable.  Linux sees /dev/ttyACM0
 *				this code goes to "configured"
 * So this code runs, but nothing is listening yet on the
 *  linux side.  In fact when I try to run picocom, it rejects
 *  it the first few times.
 * Linux seems to require about 15 seconds to allow a connection.
 * This may be because the device disconnected quite recently,
 * but who knows.
 * So I miss the first 15 seconds of output.
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

    fd_usb = serial_begin ( SERIAL_USB, 999 );
    // usb_wait ();
    // usb_xxx ();
    //usb_cdcacm_wait ();
    printf ( "-- USB Happy and Starting\n");
    printf ( "Waiting for input from USB\n" );

    /* For about 5 seconds, I get chatter from USB
     * as follows, it repeats precisely in these groups.
     * What could it be ???
     * The whole business lasts about 13 seconds.
     * AHA !!  This is linux trying to talk to the USB device
     * when it first connects.  This corresponds to the 15 seconds
     * when I cannot connect picocom and get:
     * cannot open /dev/ttyACM0: Device or resource busy
     * I also see the log message:
     * ModemManager[833]: <warn>  Could not grab port (tty/ttyACM0)
     * So ModemManager is poking away at this new device.
     * And sending the infamout "AT" command.
     * I **NEVER** plan to have modem on a serial port, so I should
     * just figure out how to nuke modemmanager.

	Got input from USB: 00000041 A
	Got input from USB: 00000054 T
	Got input from USB: 0000000A

	Got input from USB: 00000041 A
	Got input from USB: 00000054 T
	Got input from USB: 0000000A

	Got input from USB: 00000041 A
	Got input from USB: 00000054 T
	Got input from USB: 0000000A

	Got input from USB: 0000007E ~
	Got input from USB: 00000000
	Got input from USB: 00000078 x
	Got input from USB: 000000F0 �
	Got input from USB: 0000007E ~

	Got input from USB: 0000007E ~
	Got input from USB: 00000000
	Got input from USB: 00000078 x
	Got input from USB: 000000F0 �
	Got input from USB: 0000007E ~
     */

    for ( ;; ) {
	char xx[2];
	c = serial_getc ( fd_usb );
	xx[0] = c;
	xx[1] = '\0';
	printf ( "Got input from USB: %h %s\n", c, xx );
    }

    for ( count = 1; ; count++ ) {

	toggleLED();

	delay ( 1000 );

        serial_printf ( fd_usb, "Count = %d\n", count );
    }
}

/* THE END */

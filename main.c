// Tests the SysTick enable/disable functions

#include <unwired.h>
#include <libmaple/systick.h>

// tjt -- we ain't got no button
// so we code something similar using a keystroke

int enable = 1;

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
    serial_puts ( fds, "-- Starting\n");

    // With the original Maple code, my linux system
    // will identify this as /dev/ttyACM0
    //  kernel: usb 1-1.1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
    //  kernel: usb 1-1.1.1: Product: Maple
    //  kernel: usb 1-1.1.1: Manufacturer: LeafLabs
    //  kernel: cdc_acm 1-1.1.1:1.0: ttyACM0: USB ACM device

    fd = serial_begin ( SERIAL_USB, 999 );
    serial_puts ( fd, "-- USB Starting\n");

    serial_puts ( fds, "-- after USB serial begin\n");
    serial_puts ( fds, "USB fd = " );
    serial_print_num ( fds, fd );
    serial_putc ( fds, '\n' );

    for ( ;; ) {

	toggleLED();

#ifdef notdef
	// An artificial delay
	// This works, but it is really fast.
	for(i = 0; i < 150000; i++)
	    ;
#endif
	delay ( 400 );

	serial_puts ( fds, "-- after delay\n");

	count = millis ();
        serial_puts ( fd, "Systick count = " );
	serial_puts ( fds, "-- after 1\n");
	serial_print_num ( fd, count );
	serial_puts ( fds, "-- after 2\n");
	serial_putc ( fd, '\n' );

	serial_puts ( fds, "-- after systick message\n");

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

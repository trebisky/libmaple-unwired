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
    int c;
    int count;
    int i;

    pinMode(BOARD_LED_PIN, OUTPUT);

    // fd = serial_begin ( SERIAL_1, 115200 );

    // With the original Maple code, my linux system
    // will identify this as /dev/ttyACM0
    //  kernel: usb 1-1.1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
    //  kernel: usb 1-1.1.1: Product: Maple
    //  kernel: usb 1-1.1.1: Manufacturer: LeafLabs
    //  kernel: cdc_acm 1-1.1.1:1.0: ttyACM0: USB ACM device

    fd = serial_begin ( SERIAL_USB, 999 );

    for ( ;; ) {

	toggleLED();

#ifdef notdef
	// An artificial delay
	// This works, but it is really fast.
	for(i = 0; i < 150000; i++)
	    ;
#endif
	delay ( 400 );

	count = millis ();
        serial_puts ( fd, "Systick count = " );
	serial_print_num ( fd, count );
	serial_putc ( fd, '\n' );

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

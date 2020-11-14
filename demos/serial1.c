// Simple serial port test
// spit out endless "x" characters.
// Also LED blinking to reassure us the code is running.

// The STM32F103 has 3 serial ports, so we can call
// serial_begin with 1,2, or 3.

// #include <wirish/wirish.h>
#include <unwired.h>

void
echo_test ( int fd )
{
    for ( ;; ) {
	while ( ! serial_available ( fd ) )
	    ;
        serial_putc ( fd, serial_getc(fd) );
    }
}

void
flood_test ( int fd )
{
    for ( ;; ) {
        serial_write ( fd, 'x' );
	togglePin(BOARD_LED_PIN);
	delay ( 50 );
    }
}

void 
hello_test ( int fd )
{
    int count;

    for ( count=1; ; count++ ) {
        serial_printf ( fd, "Hello out there %d\n", count );
        // serial_puts ( fd, "Hello out there " );
	// serial_print_num ( fd, count );
	// serial_putc ( fd, '\n' );

	togglePin(BOARD_LED_PIN);
	// delay ( 1000 );
	delay ( 500 );
    }
}

void
main(void)
{
    int fd;

    pinMode(BOARD_LED_PIN, OUTPUT);

    // Serial1.begin(115200);
    fd = serial_begin ( SERIAL_1, 115200 );

    // echo_test ( fd );
    // flood_test ( fd );
    hello_test ( fd );

}

/* THE END */

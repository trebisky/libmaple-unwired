// Simple serial port test
// spit out endless "x" characters.
// Also LED blinking to reassure us the code is running.

// The STM32F103 has 3 serial ports, so we can call
// serial_begin with 1,2, or 3.

// #include <wirish/wirish.h>
#include <unwired.h>

void
echo ( int fd )
{
    for ( ;; ) {
	while ( ! serial_available ( fd ) )
	    ;
        serial_putc ( fd, serial_getc(fd) );
    }
}

void
main(void)
{
    int fd;
    int count;

    pinMode(BOARD_LED_PIN, OUTPUT);

    // Serial1.begin(115200);
    fd = serial_begin ( SERIAL_1, 115200 );
    echo ( fd );

    for ( count=1; ; count++ ) {
        // serial_write ( fd, 'x' );
        serial_puts ( fd, "Hello out there " );
	serial_print_num ( fd, count );
	serial_putc ( fd, '\n' );

	togglePin(BOARD_LED_PIN);
	delay ( 1000 );
    }
}

/* THE END */

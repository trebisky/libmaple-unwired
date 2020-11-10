// Now we want to run this on a real maple board
// rather than a blue pill.
//
// Set the target to flash rather than jtag
// to get the link to start at 0x08005000
// I.e. build with "make flash"
//
// Use "make dfu" to send to a board in permanent
// boot loader mode.

#include <io.h>
#include <time.h>

// writing 1 turns the LED on.

void
main ( void )
{
	pinMode(BOARD_LED_PIN, OUTPUT);

	for ( ;; ) {
	    // togglePin(BOARD_LED_PIN);
	    digitalWrite ( BOARD_LED_PIN, 0 );
	    delay ( 999 );

	    digitalWrite ( BOARD_LED_PIN, 1 );
	    delay ( 100 );
	    digitalWrite ( BOARD_LED_PIN, 0 );
	    delay ( 200 );
	    digitalWrite ( BOARD_LED_PIN, 1 );
	    delay ( 100 );
	    digitalWrite ( BOARD_LED_PIN, 0 );

	    delay ( 999 );
	    digitalWrite ( BOARD_LED_PIN, 1 );
	    delay ( 999 );
	}
}

/* THE END */

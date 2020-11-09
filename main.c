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

// #define	BLINK_PIN	BOARD_LED_PIN

// This is PA5 on the Maple
// See: wired/wirish_ORIG/boards/maple/include/board/board.h
// #define	BLINK_PIN	33
#define	BLINK_PIN	13

// This still does not work ...

void
main ( void )
{
	pinMode(BLINK_PIN, OUTPUT);

	for ( ;; ) {
	    togglePin(BLINK_PIN);
	    delay ( 100 );
	}
}

/* THE END */

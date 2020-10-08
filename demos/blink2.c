// #include <wirish/wirish.h>
#include <unwired.h>

void
main ( void )
{
	pinMode(BOARD_LED_PIN, OUTPUT);

	for ( ;; ) {
	    togglePin(BOARD_LED_PIN);
	    delay ( 100 );
	}
}

/* THE END */

/*
 * Copyright (C) 2020  Tom Trebisky  <tom@mmto.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */
/* Mine depth monitor
 *
 *  5-22-2021
 *
 * This uses either a BMP180 or a BMP390 pressure/temperature sensor
 *  along with an SSD1306 OLED display.
 */

/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */

#include <unwired.h>
#include <util.h>
#include <i2c.h>
#include <iwdg.h>

#include <string.h>

void ssd_init ( struct i2c * );
void ssd_clear_all ( void );
void ssd_display ( void );
void ssd_set_pixel ( int, int );
void ssd_clear_pixel ( int, int );
void ssd_xy ( int, int );
void ssd_putc ( int );
void ssd_puts ( char * );
void ssd_set_textsize ( int );
void ssd_text ( int, int, char * );

void bmp_init ( struct i2c * );
void bmp_diag1 ( struct i2c * );
int bmp_tf ( struct i2c * );
int bmp_press ( struct i2c * );

void bmpx_init ( struct i2c * );

#define SEA_PRESSURE	101325

int
calc_alt ( int p )
{
	int alt;

	alt = SEA_PRESSURE - p;
	alt *= 277;
	alt /= 1000;
	return alt;
}

/* =========================================== */
/* =========================================== */

void
depth1 ( void )
{
	char *depth = "XYZ";
	char *status = "Running";

	ssd_clear_all ();
	ssd_set_textsize ( 2 );
	ssd_text ( 0, 0, status );

	ssd_set_textsize ( 4 );
	ssd_text (12, 20, depth );
	ssd_display ();
}

void
depth2 ( void )
{
	int x, y;
	int width = 64;
	int h = 32;

	ssd_clear_all ();
	for ( x=0; x<width; x++ )
	    for ( y=0; y<h; y++ )
		ssd_set_pixel ( x, y );

	ssd_display ();
}

void
depth3 ( void )
{
	int num = 64;
	int ch;

	ssd_clear_all ();

	ch = '1';
	while ( num-- ) {
	    ssd_putc ( ch );
	    ssd_putc ( '-' );
	    ch++;
	    if ( ch > '9' )
		ch = '0';
	}

	ssd_display ();
}

/* ================== */

static void
show_val_10 ( int val, int x, int y, int size )
{
	char buf[12];
	int n;
	int digit;

	digit = '0' + val % 10;
	val /= 10;

	sprintf ( buf, "%d", val );
	n = strlen ( buf );
	buf[n] = '.';
	buf[n+1] = digit;
	buf[n+2] = '\0';

	ssd_set_textsize ( size );
	ssd_xy ( x, y );
	ssd_puts ( buf );
}

/* As above, but calculate "x" to put the value
 * at the right edge of the screen
 */
static void
show_val_10_rj ( int val, int y, int size )
{
	char buf[12];
	int n;
	int digit;
	int xloc;

	digit = '0' + val % 10;
	val /= 10;

	sprintf ( buf, "%d", val );
	n = strlen ( buf );
	buf[n] = '.';
	buf[n+1] = digit;
	buf[n+2] = '\0';

	xloc = (n+2) * 6 * size;
	xloc = 128 - xloc;

	ssd_set_textsize ( size );
	ssd_xy ( xloc, y );
	ssd_puts ( buf );
}

static void
show_val ( int val, int x, int y, int size )
{
	char buf[12];

	sprintf ( buf, "%d", val );

	ssd_set_textsize ( size );
	ssd_xy ( x, y );
	ssd_puts ( buf );
}

/* As above, but calculate "x" to put the value
 * at the right edge of the screen
 */
static void
show_val_rj ( int val, int y, int size )
{
	char buf[12];
	int n;
	int xloc;

	sprintf ( buf, "%d", val );
	n = strlen ( buf );

	xloc = n * 6 * size;
	xloc = 128 - xloc;

	ssd_set_textsize ( size );
	ssd_xy ( xloc, y );
	ssd_puts ( buf );
}

int starting_pressure;

/* The BMP180 datasheet tells us that a change in pressure of
 * 1 Pa is an 0.0843 meter change
 * .0843 * 3.28084 = .2766 feet change
 * We use 0.277 below via scaled arithmetic.
 */

static void
one_deep ( struct i2c *ip )
{
	int temp;
	int press;
	int deep;
	int alt;

	temp = bmp_tf ( ip );
	press = bmp_press ( ip );
	// printf ( "Current pressure: %d\n", press );
	alt = calc_alt ( press );

	deep = press - starting_pressure;
	deep *= 277;
	deep /= 1000;

	ssd_clear_all ();

	show_val_10_rj ( temp, 0, 2 );
	show_val ( deep, 0, 16, 4 );
	show_val ( press, 0, 56, 1 );
	show_val_rj ( alt, 48, 2 );
	deep++;

	ssd_display ();
}

void
depth ( struct i2c *ip )
{
	int p1;
	int t1;
	int alt;

	printf ( "Running screen demo on BMP180\n" );
	/* The bmp180 driver requires us to read
	 * both T and P.
	 */
	t1 = bmp_tf ( ip );
	p1 = bmp_press ( ip );
	printf ( "Starting pressure: %d\n", p1 );

	delay ( 2000 );
	t1 = bmp_tf ( ip );
	p1 = bmp_press ( ip );
	printf ( "Starting pressure: %d\n", p1 );

	/* Save this as reference for depth measurements */
	starting_pressure = p1;

	/* Topo map shows Castellon at 2400 feet.
	 * This gives 2375 on 5-21-2021.
	 */
	printf ( "Elevation: %d feet\n", calc_alt ( p1 ) );

	for ( ;; ) {
	    delay ( 2000 );
	    one_deep ( ip );
	}
}

void
main(void)
{
    int fd;
    int fd_gps;
    struct i2c *ip;

    pinMode(BOARD_LED_PIN, OUTPUT);

    /* Turn on LED */
    toggleLED();
    toggleLED();

    fd = serial_begin ( SERIAL_1, 115200 );
    set_std_serial ( fd );

    printf ( "\n" );
    printf ( "-- BOOTED -- off we go\n" );

    // fd_gps = serial_begin ( SERIAL_2, 9600 );

    /* D30 is sda, D29 is sclk */
    // ip = i2c_gpio_new ( D30, D29 );

    /* For blue pill */
    ip = i2c_gpio_new ( PB11, PB10 );

    if ( ! ip ) {
	printf ( "Cannot set up GPIO iic\n" );
	spin ();
    }

    printf ( "Initalize SSD\n" );
    ssd_init ( ip );

    printf ( "Initalize BMP\n" );
    bmp_init ( ip );
    bmpx_init ( ip );

    // gps_fail ( "INIT" );
    // ssd_clear_all ();
    // ssd_display ();

    /* Sometimes i2c locks up, and until I find and
     * fix that bug, this helps a lot.
     * (the above was prior to switching to bit/bang i2c)
     *
     * The iwdt is fed by a 40 khz clock.
     * With a prescaler of 4, the iwdg counts at 4 khz.
     * The reload register is only 12 bits (max value 0xfff)
     * So this limits the total timeout to 409.6 milliseconds.
     * We want about 2 seconds.  So use a 64 prescaler, which
     * means the dog ticks at 1.6 ms and a preload of
     * 2000 / 1.6 = 1250 gives us 2 seconds.
     */
    // iwdg_init(IWDG_PRE_64, 1250);

    // bmp_diag1 ( ip );

    // gps ( fd_gps );
    depth ( ip );
}
/* THE END */

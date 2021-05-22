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
/* GPS Altimeter
 *  10-13-2020
 *
 * Keep using serial port 1 as a debug console.
 * Connect a Sparkfun SAM-M8Q to serial port 2.
 *
 * This also needs the i2c_lcd.c driver
 * and you will need to change a line in build-targets.mk
 * so it will compile and link both files.
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
void ssd_set_textsize ( int );
void ssd_text ( int, int, char * );

// #define LCD_DISPLAY
#define SSD_DISPLAY

struct i2c *ip;

#ifdef LCD_DISPLAY
void lcd_begin ( void );
void lcd_reinit ( void );
void lcd_msg ( struct i2c *, char * );
void lcd_msg2 ( struct i2c *, char * );
#endif

#ifdef notdef
/* I see this without satellite lock */
$GNGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*2E
$GNGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*2E
$GPGSV,1,1,00*79
$GLGSV,1,1,01,71,,,28*68
$GNGLL,,,,,013104.00,V,N*53
$GNRMC,013105.00,V,,,,,,,141020,,,N*63
$GNVTG,,,,,,,,,N*2E
$GNGGA,013105.00,,,,,0,00,99.99,,,,,,*7E

/* Once we get lock, I see this */
$GNGSA,A,3,04,26,09,,,,,,,,,,3.09,1.59,2.65*13
$GNGSA,A,3,71,69,,,,,,,,,,,3.09,1.59,2.65*13
$GPGSV,2,1,07,04,67,003,25,06,04,290,,07,25,263,18,08,06,156,*77
$GPGSV,2,2,07,09,37,316,22,16,60,078,18,26,29,046,21*4E
$GLGSV,2,1,07,69,37,138,27,70,76,353,,71,24,329,28,73,48,240,*6A
$GLGSV,2,2,07,74,02,224,,79,15,034,16,80,67,359,*5B
$GNGLL,3215.76915,N,11102.91418,W,014125.00,A,A*69
$GNRMC,014126.00,A,3215.76919,N,11102.91426,W,0.309,,141020,,,A*7E
$GNVTG,,T,,M,0.309,N,0.573,K,A*36
$GNGGA,014126.00,3215.76919,N,11102.91426,W,1,05,1.59,740.6,M,-28.4,M,,*70
#endif

/* GP is GPS info
 * GL is GLONASS info
 * GPGSV is GPS satellites in view
 * GLGSV is Glonass satellites in view
 * GNGGA is a position, including a UT timestamp and elevation.
 */
/* NMEA must be no longer than 80 visible bytes, plus cr-lf terminator */
#define NMEA_MAX	82

/* copy from src up to but not including comma */
void
copy_to_comma ( char *dest, char *src )
{
	char *p;
	char *q;

	q = dest;
	for ( p = src; *p; p++ ) {
	    if ( *p == ',' )
		break;
	    *q++ = *p;
	}
	*q = '\0';
}

/* Convert a string like "723.0" in meters
 * to an integer elevation in feet like 2372.05
 *  1 meter = 3.28084 feet
 * My house is at approximately 2400 feet.
 *  which is 731.5 meters
 */
int
m2f ( char *alt )
{
	char *p;
	int m = 0;
	int rv;

	for ( p=alt; *p; p++ ) {
	    if ( *p == '.' )
		continue;
	    m = m*10 + (*p-'0');
	}

	rv = m * 3;
	m /= 10;
	rv += m * 2;
	m /= 10;
	rv += m * 8;
	m /= 10;
	m /= 10;
	rv += m * 8;
	m /= 10;
	rv += m * 4;

	return (rv+5) / 10;
}

/* Arizona */
#define TIME_ZONE	-7

void
ut2lt ( char *time )
{
	char *p;
	int hour;
	int d1, d2;

	for ( p=time; *p; p++ )
	    if ( *p == '.' )
		*p = '\0';

	hour = time[0] - '0';
	hour = hour*10 + time[1] - '0';
	hour += TIME_ZONE;
	if ( hour < 0 ) hour += 24;
	if ( hour > 23 ) hour -= 24;
	d2 = hour % 10;
	d1 = hour / 10;
	time[0] = d1 + '0';
	time[1] = d2 + '0';
}

void
colons ( char *ctime, char *time )
{
    ctime[0] = time[0];
    ctime[1] = time[1];
    ctime[2] = ':';

    ctime[3] = time[2];
    ctime[4] = time[3];
    ctime[5] = ':';

    ctime[6] = time[4];
    ctime[7] = time[5];
    ctime[8] = '\0';
}

/*
 * GNGGA has the altitude, the fields are:
$GNGGA,
1) 014126.00,    -- utc time hhmmss.sss
2) 3215.76919,N,  - latitude
4) 11102.91426,W, - longitude
6) 1,		- quality (1 = valid)
7) 05,		- number of satellites used
8) 1.59,		- HDOP (horizontal dilution of precision)
9) 740.6,M,	- altitude in meters
10) -28.4,M,	- Geoidal Separation in meters
11) ,		- diff GPS station used (null field here)
12) *70		- checksum

--- Need to be able to handle a line like this:
$GNGGA,224159.00,,,,,0,04,18.03,,,,,,*7F
 */

void
nmea_get ( char *buf, char *line, int count )
{
	int ccount;	/* count commas */
	char *p;

	if ( count == 0 ) {
	    copy_to_comma ( buf, line );
	    return;
	}

	ccount = 0;
	for ( p = line; *p; p++ ) {
	    if ( *p == ',' ) {
		++ccount;
		if ( ccount == count )
		    copy_to_comma ( buf, p+1 );
	    }
	}
}

/* Add blanks to a string to make it
 * a certain length (must be space in
 * the buffer of course)
 */
void
pad_to ( char *buf, int n )
{
	int i;
	int pad = 0;
	char *p = buf;

	while ( n-- ) {
	    if ( pad )
		*p++ = ' ';
	    else {
		if ( *p ) {
		    p++;
		    continue;
		}
		pad = 1;
		*p++ = ' ';
	    }
	}
	*p++ = '\0';
}

void
gps_fail ( char *msg )
{
	printf ( "GPS fail: %s\n", msg );

	ssd_clear_all ();
	ssd_set_textsize ( 4 );
	ssd_text ( 0, 0, msg );
	ssd_display ();
}

/* This gets called with every line that comes from
 * the GPS serial stream.
 */
void
gps_line ( char *line )
{
	char raw[12];
	char alt[17];
	char time[12];
	char nsat[4];

	iwdg_feed ();

	/* Complete bogus junk */
	if ( line[6] != ',' ) {
	    // gps_fail ( "FAIL1" );
	    return;
	}

	/* We only care about GNGGA lines, and
	 * we should get one per second.
	 */
	nmea_get ( raw, line, 0 );
	if ( strcmp ( raw, "$GNGGA" ) != 0 ) {
	    // Not really a fail, we expect several of these
	    // every second.
	    // gps_fail ( "FAIL2" );
	    return;
	}

	nmea_get ( nsat, line, 7 );
	    
	printf ( "GPS line: %s\n", line );

	/* Get the UT time and convert to local */
	/* When we turn this on in a metal shed,
	 * we see:
	 * $GNGGA,,,,,,0,00,99.99,,,,,,*56
	 * This will yield FAIL3
	 */
	nmea_get ( raw, line, 1 );
	if ( strlen(raw) != 9 ) {
	    gps_fail ( "FAIL3" );
	    return;
	}

	colons ( time, raw );
	ut2lt ( time );

	printf ( "Time = %s\n", time );

	/* Get the elevation im meters */
	nmea_get ( raw, line, 9 );

#ifdef LCD_DISPLAY
	if ( strlen(raw) < 1 ) {
	    strcpy ( alt, " -- ? --" );
	} else {
	    printf ( "Elev = %s\n", raw );
	    sprintf ( alt, "%d", m2f(raw) );
	    printf ( "Elev (f) = %s\n", alt );
	}

	pad_to ( alt, 14 );
	strcat ( alt, nsat );

	/* Should not need this, but ...
	 */
	lcd_reinit ();

	lcd_msg ( ip, alt );
	lcd_msg2 ( ip, time );
#endif

#ifdef SSD_DISPLAY
	if ( strlen(raw) < 1 ) {
	    strcpy ( alt, " ?" );
	} else {
	    printf ( "Elev = %s\n", raw );
	    sprintf ( alt, "%d", m2f(raw) );
	    printf ( "Elev (f) = %s\n", alt );
	}

	ssd_clear_all ();
	ssd_set_textsize ( 4 );
	ssd_text ( 0, 0, alt );
	ssd_set_textsize ( 2 );
	ssd_text ( 10, 49, time );
	ssd_display ();
#endif

//	iwdg_feed ();
	printf ( " ~~ update finished\n" );
	toggleLED();
}

void
gps ( int fd )
{
    int c;
    char buf[NMEA_MAX];
    char *p;

    p = buf;
    for ( ;; ) {
	c = serial_getc ( fd );
	if ( c == '\n' ) {
	    *p = '\0';
	    if ( p != buf )
		gps_line ( buf );
	    p = buf;
	} else
	    *p++ = c;
    }
}

void
main(void)
{
    int fd;
    int fd_gps;

    pinMode(BOARD_LED_PIN, OUTPUT);

    /* Turn on LED */
    toggleLED();
    toggleLED();

    fd = serial_begin ( SERIAL_1, 115200 );
    set_std_serial ( fd );

    fd_gps = serial_begin ( SERIAL_2, 9600 );

    /* D30 is sda, D29 is sclk */
    // ip = i2c_gpio_new ( D30, D29 );

    /* For blue pill */
    ip = i2c_gpio_new ( PB11, PB10 );

    if ( ! ip ) {
	printf ( "Cannot set up GPIO iic\n" );
	spin ();
    }

#ifdef SSD_DISPLAY
    printf ( "Initalize SSD\n" );
    ssd_init ( ip );
    gps_fail ( "INIT" );
    // ssd_clear_all ();
    // ssd_display ();
#endif

#ifdef LCD_DISPLAY
    lcd_begin ();
    lcd_msg ( ip, "Starting ..." );
#endif

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
    iwdg_init(IWDG_PRE_64, 1250);

    printf ( "-- BOOTED -- off we go\n" );

    gps ( fd_gps );
}
/* THE END */

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
 */
/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */

#include <unwired.h>
#include <libmaple/util.h>
#include <libmaple/i2c.h>

#include <string.h>

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
 * GNGGA is a position, including a UT timestamp.
 *
 * GNGGA has the altitude, the fields are:
$GNGGA,
014126.00,    -- utc time hhmmss.sss
3215.76919,N,  - latitude
11102.91426,W, - longitude
1,		- quality (1 = valid)
05,		- number of satellites used
1.59,		- HDOP (horizontal dilution of precision)
740.6,M,	- altitude in meters
-28.4,M,	- Geoidal Separation in meters
,		- diff GPS station used (null field here)
*70		- checksum
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

void
gps_line ( char *line )
{
	char raw[12];
	char alt[12];
	char time[16];
	char *p;
	int ccount;	/* count commas */

	if ( line[6] != ',' )
	    return;
	    
	line[6] = '\0';
	if ( strcmp ( line, "$GNGGA" ) != 0 )
	    return;
	line[6] = ',';

	printf ( "%s\n", line );

	copy_to_comma ( raw, &line[7] );
	colons ( time, raw );
	ut2lt ( time );
	printf ( "Time = %s\n", time );

	ccount = 0;
	for ( p = line; *p; p++ ) {
	    if ( *p == ',' ) {
		++ccount;
		if ( ccount == 9 )
		    copy_to_comma ( raw, p+1 );
	    }
	}
	printf ( "Elev = %s\n", raw );
	sprintf ( alt, "%d", m2f(raw) );
	printf ( "Elev (f) = %s\n", alt );
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

    fd = serial_begin ( SERIAL_1, 115200 );
    set_std_serial ( fd );

    fd_gps = serial_begin ( SERIAL_2, 9600 );

    // i2c_master_enable ( I2C2, 0);

    printf ( "-- BOOTED -- off we go\n" );

    gps ( fd_gps );
}
/* THE END */
/*
 * Copyright (C) 2020  Tom Trebisky  <tom@mmto.org>
 * Copyright (C) 2016  Tom Trebisky  <tom@mmto.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */
/* BMP 180 Pressure and Temperature sensor (Bosch)
 *  This code was first developed for the ESP8266 using
 *  a bit-bang software iic scheme.  That iic code was
 *  then moved to Kyu.
 *  Now, here we have it for the STM32F103
 *  10-13-2020
 */
/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */

#ifdef notdef
/* Once this gets going, it spits out:
 */

Temp = 777 -- Pressure (mb*100, sea level) = 101275
Temp = 777 -- Pressure (mb*100, sea level) = 101273
Temp = 777 -- Pressure (mb*100, sea level) = 101275
Temp = 777 -- Pressure (mb*100, sea level) = 101269
Temp = 777 -- Pressure (mb*100, sea level) = 101275
Temp = 777 -- Pressure (mb*100, sea level) = 101269
Temp = 777 -- Pressure (mb*100, sea level) = 101267

The temperature in this room is 78F according to my thermometer
and the pressure at the airport, adjusted to sea level (as they
post it) is currently 1012.0 mb, so these values seem just right.
#endif

#include <unwired.h>
#include <libmaple/util.h>
#include <libmaple/i2c.h>

/* totally bogus to simulate Kyu API,
 *  --- we will clean up later.
 */
struct i2c {
    int bogus;
};

void bmp_diag1 ( struct i2c *ip );
static void bmp_cals ( struct i2c *ip, unsigned short *buf );
static int bmp_temp ( struct i2c *ip );
static int bmp_pressure ( struct i2c *ip );
int convert ( int rawt, int rawp );

/* Notes on the libmaple i2c API.
 * It is all about calls to i2c_master_xfer(dev,msgs,num,timeout)
 *  dev is the I2C device
 *  msgs is one or more messages to send/receive
 *  num is how many messages
 *  timeout is a timeout, or 0 for no timeout.
 *  return is:
 *         0 on success,
 *         I2C_ERROR_PROTOCOL if there was a protocol error,
 *         I2C_ERROR_TIMEOUT if the transfer timed out.
 */

void
bmp_once ( struct i2c *ip )
{
	int t, p;

	t = bmp_temp ( ip );
	p = bmp_pressure ( ip );
	//os_printf ( "temp = %d\n", val );
	//os_printf ( "pressure = %d\n", val );
	//os_printf ( "raw T, P = %d  %d\n", t, p );

	convert ( t, p );
}

#define NCALS	11

struct bmp_cals {
	short ac1;
	short ac2;
	short ac3;
	unsigned short ac4;
	unsigned short ac5;
	unsigned short ac6;
	short b1;
	short b2;
	short mb;
	short mc;
	short md;
} bmp_cal;

#define	AC1	bmp_cal.ac1
#define	AC2	bmp_cal.ac2
#define	AC3	bmp_cal.ac3
#define	AC4	bmp_cal.ac4
#define	AC5	bmp_cal.ac5
#define	AC6	bmp_cal.ac6
#define	B1	bmp_cal.b1
#define	B2	bmp_cal.b2
#define	MB	bmp_cal.mb	/* never used */
#define	MC	bmp_cal.mc
#define	MD	bmp_cal.md

void
bmp_test ( void )
{
	struct i2c *ip = (struct i2c *) 0;

	bmp_diag1 ( ip );

	bmp_cals ( ip, (unsigned short *) &bmp_cal );

	for ( ;; ) {
	    bmp_once ( ip );
	    delay ( 1000 );
	}
}

void
main(void)
{
    int fd;

    fd = serial_begin ( SERIAL_1, 115200 );
    set_std_serial ( fd );

    i2c_master_enable ( I2C2, 0);

    printf ( "-- BOOTED -- off we go\n" );

    bmp_test ();
}

#define I2C_TIMEOUT	3000

#ifdef notdef
/* These are in i2c.h */
#define I2C_MSG_READ            0x1
#define I2C_MSG_10BIT_ADDR      0x2
#endif

/* Also bogus for now.
 * once things are working, absorb this into lcd_write()
 * This is a kyu API.
 */
int
i2c_send ( struct i2c *ip, int addr, unsigned char *buf, int count )
{
    i2c_msg write_msg;
    int stat;

    // printf ( "Start i2c_send\n" );

    write_msg.addr = addr;
    write_msg.flags = 0; // write, 7 bit address
    write_msg.data = buf;
    write_msg.length = count;
    write_msg.xferred = 0;

    stat = i2c_master_xfer ( I2C2, &write_msg, 1, I2C_TIMEOUT);
    if ( stat ) {
	printf ( "Finished i2c_send: returned: %d\n", stat );
	printf ( "Trouble with i2c, spinning\n" );
	for ( ;; ) ;
    }
    return 0;
}

int
i2c_recv ( struct i2c *ip, int addr, unsigned char *buf, int count )
{
    i2c_msg read_msg;
    int stat;

    // printf ( "Start i2c_recv\n" );

    read_msg.addr = addr;
    read_msg.flags = I2C_MSG_READ;
    read_msg.data = buf;
    read_msg.length = count;
    read_msg.xferred = 0;

    stat = i2c_master_xfer ( I2C2, &read_msg, 1, I2C_TIMEOUT);
    if ( stat ) {
	printf ( "Finished i2c_recv: returned: %d\n", stat );
	printf ( "Trouble with i2c, spinning\n" );
	for ( ;; ) ;
    }
    return 0;
}

/* ---------------------------------------------- */
/* ---------------------------------------------- */

/* Some routines from Kyu/bbb/i2c.c */

void
i2c_read_reg_n ( struct i2c *ip, int addr, int reg, unsigned char *buf, int n )
{
        buf[0] = reg;

        if ( i2c_send ( ip, addr, buf, 1 ) )
            printf ( "send Trouble\n" );

        if ( i2c_recv ( ip, addr, buf, n ) )
            printf ( "recv Trouble\n" );
}

int
i2c_read_8 ( struct i2c *ip, int addr, int reg )
{
        char buf[1];

        i2c_read_reg_n ( ip, addr, reg, buf, 1 );

        return buf[0] & 0xff;
}

int
i2c_read_16 ( struct i2c *ip, int addr, int reg )
{
        int rv;
        unsigned char buf[2];

        buf[0] = reg;

        i2c_read_reg_n ( ip, addr, reg, buf, 2 );

        rv = buf[0] << 8;
        rv |= buf[1];

        return rv;
}

int
i2c_read_24 ( struct i2c *ip, int addr, int reg )
{
        int rv;
        unsigned char buf[3];

        buf[0] = reg;

        i2c_read_reg_n ( ip, addr, reg, buf, 3 );

        rv =  buf[0] << 16;
        rv |= buf[1] << 8;
        rv |= buf[2];

        return rv;
}

/* Code below here is almost unchanged from Kyu
 */

/* ---------------------------------------------- */
/* ---------------------------------------------- */

#define  BMP_ADDR 0x77

#define  REG_CALS		0xAA
#define  REG_CONTROL	0xF4
#define  REG_RESULT		0xF6
#define  REG_ID			0xD0

#define  CMD_TEMP 0x2E
#define  TDELAY 4500

/* OSS can have values 0, 1, 2, 3
 * representing internal sampling of 1, 2, 4, or 8 values
 * note that sampling more will use more power.
 *  (since each conversion takes about the same dose of power)
 */

#define  OSS_ULP	0			/* 16 bit - Ultra Low Power */
#define  CMD_P_ULP  0x34
#define  DELAY_ULP  4500
#define  SHIFT_ULP  8

#define  OSS_STD	1			/* 17 bit - Standard */
#define  CMD_P_STD  0x74
#define  DELAY_STD  7500
#define  SHIFT_STD	7

#define  OSS_HR		2			/* 18 bit - High Resolution */
#define  CMD_P_HR   0xB4
#define  DELAY_HR   13500
#define  SHIFT_HR	6

#define  OSS_UHR	3			/* 19 bit - Ultra High Resolution */
#define  CMD_P_UHR  0xF4
#define  DELAY_UHR  25500
#define  SHIFT_UHR	5

/* This chooses from the above */
#define  OSS 		OSS_STD
#define  CMD_PRESS	CMD_P_STD
#define  PDELAY 	DELAY_STD
#define  PSHIFT 	SHIFT_STD
// #define  PSHIFT 	(8-OSS)

/* ---------------------------------------------- */
/* ---------------------------------------------- */
/* BMP180 specific routines --- */

/* Note that b5 is set in comv_temp and used in conv_pressure */
int b5;

/* Yields temperature in 0.1 degrees C */
int
conv_temp ( int raw )
{
	int x1, x2;

	x1 = ((raw - AC6) * AC5) >> 15;
	x2 = MC * 2048 / (x1 + MD);
	b5 = x1 + x2;
	return (b5 + 8) >> 4;
}

/* Yields pressure in Pa */
int
conv_pressure ( int raw )
{
	int b3, b6;
	unsigned long b4, b7;
	int x1, x2, x3;
	int p;

	b6 = b5 - 4000;
	x1 = B2 * (b6 * b6 / 4096) / 2048;
	x2 = AC2 * b6 / 2048;
	x3 = x1 + x2;
	b3 = ( ((AC1 * 4 + x3) << OSS) + 2) / 4;
	x1 = AC3 * b6 / 8192;
	x2 = (B1 * (b6 * b6 / 4096)) / 65536;
	x3 = (x1 + x2 + 2) / 4;

	b4 = AC4 * (x3 + 32768) / 32768;
	b7 = (raw - b3) * (50000 >> OSS);

	p = (b7 / b4) * 2;
	x1 = (p / 256) * (p / 256);
	x1 = (x1 * 3038) / 65536;
	x2 = (-7357 * p) / 65536;
	p += (x1 + x2 + 3791) / 16;

	return p;
}

// #define MB_TUCSON	84.0
#define MB_TUCSON	8400

int
convert ( int rawt, int rawp )
{
	int t;
	int p;
	//int t2;
	//int p2;
	int tf;
	int pmb_sea;

	// double tc;
	// double tf;
	// double pmb;
	// double pmb_sea;

	t = conv_temp ( rawt );
	//t2 = conv_tempX ( rawt );
	// tc = t / 10.0;
	// tf = 32.0 + tc * 1.8;
	tf = t * 18;
	tf = 320 + tf / 10;

	// os_printf ( "Temp = %d\n", t );
	// os_printf ( "Temp (C) = %.3f\n", tc );
	// os_printf ( "Temp (F) = %.3f\n", tf );
	// os_printf ( "Temp = %d (%d)\n", t, tf );

	p = conv_pressure ( rawp );
	// p2 = conv_pressureX ( rawp, OSS );
	// pmb = p / 100.0;
	// pmb_sea = pmb + MB_TUCSON;
	pmb_sea = p + MB_TUCSON;

	// os_printf ( "Pressure (Pa) = %d\n", p );
	// os_printf ( "Pressure (mb) = %.2f\n", pmb );
	// os_printf ( "Pressure (mb, sea level) = %.2f\n", pmb_sea );
	// os_printf ( "Pressure (mb*100, sea level) = %d\n", pmb_sea );

	printf ( "Temp = %d -- Pressure (mb*100, sea level) = %d\n", tf, pmb_sea );
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

/* ID register -- should always yield 0x55
 */
static int
bmp_id ( struct i2c *ip )
{
	return i2c_read_8 ( ip, BMP_ADDR, REG_ID );
}

/* XXX It is possible that the maple API can handle these
'* back to back pairs of transfers in one call.
 */

static int
bmp_temp ( struct i2c *ip )
{
	int rv;
	char buf[2];

	// (void) iic_write ( BMP_ADDR, REG_CONTROL, CMD_TEMP );
	// i2c_write_8 ( ip, BMP_ADDR, REG_CONTROL, CMD_TEMP );
	buf[0] = REG_CONTROL;
	buf[1] = CMD_TEMP;
	i2c_send ( ip, BMP_ADDR, buf, 2 );

	delay_us ( TDELAY );

	// rv = iic_read_16 ( BMP_ADDR, REG_RESULT );
	rv = i2c_read_16 ( ip, BMP_ADDR, REG_RESULT );

	return rv;
}

/* Pressure reads 3 bytes
 */

static int
bmp_pressure ( struct i2c *ip )
{
	int rv;
	char buf[2];

	// (void) iic_write ( BMP_ADDR, REG_CONTROL, CMD_PRESS );
	// i2c_write_8 ( ip, BMP_ADDR, REG_CONTROL, CMD_PRESS );
	buf[0] = REG_CONTROL;
	buf[1] = CMD_PRESS;
	i2c_send ( ip, BMP_ADDR, buf, 2 );

	delay_us ( PDELAY );

	// rv = iic_read_24 ( BMP_ADDR, REG_RESULT );
	rv = i2c_read_24 ( ip, BMP_ADDR, REG_RESULT );

	return rv >> PSHIFT;
}

static void
bmp_cals ( struct i2c *ip, unsigned short *buf )
{
	char bbuf[2*NCALS];
	int i, j;

	/* Note that on the ESP8266 (or any little endian machine)
	 * just placing the bytes into memory in the order read out
	 * does not yield an array of shorts.
	 * This is because the BMP180 gives the MSB first, but the
	 * ESP8266 is little endian.
	 */

	i2c_read_reg_n ( ip, BMP_ADDR, REG_CALS, bbuf, 2*NCALS );

	for ( i=0; i<NCALS; i++ ) {
	    j = i + i;
	    buf[i] = bbuf[j]<<8 | bbuf[j+1];
	}
}

static void
show_cals ( void *cals )
{
		short *calbuf = (short *) cals;
		unsigned short *calbuf_u = (unsigned short *) cals;

		printf ( "cal 1 = %d\n", calbuf[0] );
		printf ( "cal 2 = %d\n", calbuf[1] );
		printf ( "cal 3 = %d\n", calbuf[2] );
		printf ( "cal 4 = %d\n", calbuf_u[3] );
		printf ( "cal 5 = %d\n", calbuf_u[4] );
		printf ( "cal 6 = %d\n", calbuf_u[5] );
		printf ( "cal 7 = %d\n", calbuf[6] );
		printf ( "cal 8 = %d\n", calbuf[7] );
		printf ( "cal 9 = %d\n", calbuf[8] );
		printf ( "cal 10 = %d\n", calbuf[9] );
		printf ( "cal 11 = %d\n", calbuf[10] );
}

void
bmp_diag1 ( struct i2c *ip )
{
	int id;
	int tt, pp;
	unsigned short cals[11];
	int i;

	for ( i=0; i<8; i++ ) {
	    id = bmp_id ( ip );
	    printf ( "BMP ID = %x\n", id );
	}

	for ( i=0; i<8; i++ ) {
	    tt = bmp_temp ( ip );
	    printf ( "BMP temp = %d\n", tt );
	}

	for ( i=0; i<8; i++ ) {
	    id = bmp_id ( ip );
	    printf ( "BMP ID = %x\n", id );
	    tt = bmp_temp ( ip );
	    printf ( "BMP temp = %d\n", tt );
	}

	pp = bmp_pressure ( ip );
	printf ( "BMP pressure = %d\n", pp );

	bmp_cals ( ip, (unsigned short *) &bmp_cal );
	show_cals ( (void *) &bmp_cal );
}
/* THE END */

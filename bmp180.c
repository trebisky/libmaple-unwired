/*
 * Copyright (C) 2016  Tom Trebisky  <tom@mmto.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

/* bmp180 "driver"
 * from kyu/bbb/i2c_demos.c
 * (originally from my esp8266 code).
 *
 * Tom Trebisky  5-21-2021
 */

/* Hardware i2c on the stm32 is buggy and erratic, so we use
 * "software" i2c (i.e. a bit-bang low level driver).
 *
 * Here we have the Bosch BMP180
 *  pressure and temperature sensor.
 */

/* For maple-unwired */
#include <unwired.h>
#include <i2c.h>

#ifdef KYU
#include <kyu.h>
#include "i2c.h"
#endif

#define ARCH_ARM

static int bmp_temp ( struct i2c * );
static int bmp_pressure ( struct i2c * );

/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */
/* BMP 180 Pressure and Temperature sensor (Bosch)
 *  This code was first developed for the ESP8266 using
 *  a bit-bang software iic scheme.  That iic code was
 *  then moved to Kyu.  */
/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */

#define  BMP_ADDR 0x77

#define  REG_CALS		0xAA
#define  REG_CONTROL		0xF4
#define  REG_RESULT		0xF6
#define  REG_ID			0xD0

#define  CMD_TEMP 		0x2E

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
#define  PSHIFT 	SHIFT_STD
// #define  PSHIFT 	(8-OSS)

/*
#define  TDELAY		4500
#define  PDELAY 	DELAY_STD
*/

// 5-21-2021 with STM32, going fail safe.
#define  TDELAY		20000
#define  PDELAY 	20000

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

/* ---------------------------------------------- */
/* ---------------------------------------------- */

#ifdef notyet
/* 5-21-2021 */
static int
read_id ( void )
{
	char buf[2];

	buf[0] = REG_ID;
	i2c_send ( g_ip, BMP_ADDR, buf, 1 );
	i2c_recv ( g_ip, BMP_ADDR, buf, 1 );

	// id = iic_read ( BMP_ADDR, REG_ID );

	return buf[0];;
}
#endif


#ifdef OLD_BMP
/* ID register -- should always yield 0x55
 */
static int ICACHE_FLASH_ATTR
read_id ( void )
{
	int id;

	id = iic_read ( BMP_ADDR, REG_ID );

	return id;
}

/* For extra diagnostics */
static int ICACHE_FLASH_ATTR
read_idx ( void )
{
	int id;

	id = iic_readx ( BMP_ADDR, REG_ID );

	return id;
}

static int ICACHE_FLASH_ATTR
read_temp ( void )
{
	int rv;

	(void) iic_write ( BMP_ADDR, REG_CONTROL, CMD_TEMP );

	os_delay_us ( TDELAY );

	rv = iic_read_16 ( BMP_ADDR, REG_RESULT );

	return rv;
}

/* Pressure reads 3 bytes
 */

static int ICACHE_FLASH_ATTR
read_pressure ( void )
{
	int rv;

	(void) iic_write ( BMP_ADDR, REG_CONTROL, CMD_PRESS );

	os_delay_us ( PDELAY );

	rv = iic_read_24 ( BMP_ADDR, REG_RESULT );

	return rv >> PSHIFT;
}

static void ICACHE_FLASH_ATTR
read_cals ( unsigned short *buf )
{
	/* Note that on the ESP8266 just placing the bytes into
	 * memory in the order read out does not yield an array of shorts.
	 * This is because the BMP180 gives the MSB first, but the
	 * ESP8266 is little endian.  So we use our routine that gives
	 * us an array of shorts and we are happy.
	 */
	// iic_read_n ( BMP_ADDR, REG_CALS, (unsigned char *) buf, 2*NCALS );
	iic_read_16n ( BMP_ADDR, REG_CALS, buf, NCALS );
}
#endif /* OLD_BMP */

/* ---------------------------------------------- */
/* ---------------------------------------------- */

#ifdef notdef
/* This raw pressure value is already shifted by 8-oss */
int rawt = 25179;
int rawp = 77455;

int ac1 = 8240;
int ac2 = -1196;
int ac3 = -14709;
int ac4 = 32912;	/* U */
int ac5 = 24959;	/* U */
int ac6 = 16487;	/* U */
int b1 = 6515;
int b2 = 48;
int mb = -32768;
int mc = -11786;
int md = 2845;
#endif

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

#ifdef notdef
int
conv_tempX ( int raw )
{
	int x1, x2;

	x1 = (raw - ac6) * ac5 / 32768;
	x2 = mc * 2048 / (x1 + md);
	b5 = x1 + x2;
	return (b5+8) / 16;
}

int
conv_pressureX ( int raw, int oss )
{
	int b3, b6;
	unsigned long b4, b7;
	int x1, x2, x3;
	int p;

	b6 = b5 - 4000;
	x1 = b2 * (b6 * b6 / 4096) / 2048;
	x2 = ac2 * b6 / 2048;
	x3 = x1 + x2;
	b3 = ( ((ac1 * 4 + x3) << oss) + 2) / 4;
	x1 = ac3 * b6 / 8192;
	x2 = (b1 * (b6 * b6 / 4096)) / 65536;
	x3 = (x1 + x2 + 2) / 4;

	b4 = ac4 * (x3 + 32768) / 32768;
	b7 = (raw - b3) * (50000 >> oss);

	p = (b7 / b4) * 2;
	x1 = (p / 256) * (p / 256);
	x1 = (x1 * 3038) / 65536;
	x2 = (-7357 * p) / 65536;
	p += (x1 + x2 + 3791) / 16;

	return p;
}
#endif

// #define MB_TUCSON	84.0
#define MB_TUCSON	8400

/* Return temp in fahrenheit */
int
bmp_tf ( struct i2c *ip )
{
	int traw;
	int tc;
	int tf;

	traw = bmp_temp ( ip );

	// printf ( "Raw T = %d (%h)\n", traw, traw );

	//os_printf ( "temp = %d\n", val );
	//os_printf ( "pressure = %d\n", val );
	//os_printf ( "raw T, P = %d  %d\n", t, p );

	tc = conv_temp ( traw );

	tf = tc * 18;
	tf = 320 + tf / 10;
	// printf ( "tc, tf = %d %d\n", tc, tf );

	return tf;
}

/* Return pressure in pascals */
int
bmp_press ( struct i2c *ip )
{
	int praw;
	int p;

	praw = bmp_pressure ( ip );
	p = conv_pressure ( praw );

	// return p + MB_TUCSON;
	return p;
}

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

void
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
/* ---------------------------------------- */

#ifdef notyet
static void
iic_tester ( void )
{
	static int first = 1;
	static int count = 0;
	int val;

	if ( first ) {
		val = read_id ();
		os_printf ( "BMP180 id = %02x\n", val );

		read_cals ( (unsigned short *) &bmp_cal );
		show_cals ( (void *) &bmp_cal );

		// iic_write ( MCP_ADDR, MCP_DIR, 0 );		/* outputs */

		// dac_show ();

		// hdc_test ();

		first = 0;
	}

	// Scope loop
	//for ( ;; ) {
	//	val = read_id ();
	//}

	val = read_id ();
	os_printf ( "BMP180 id = %02x\n", val );

	read_cals ( (unsigned short *) &bmp_cal );
	show_cals ( (void *) &bmp_cal );

	// read_bmp ();
	// upd_mcp ();
	// dac_doodle ();
	// hdc_read ();

	/*
	if ( ++count > 10 ) {
		os_printf ( "Finished\n" );
		os_timer_disarm(&timer);
	}
	*/
}

void
iic_tester2 ( void )
{
	int i;
	int val;

	// id = iic_read ( BMP_ADDR, REG_ID );

	for ( i=0; i<4; i++ )
		iic_diag ( BMP_ADDR, REG_ID );

	for ( i=0; i<4; i++ )
		iic_diag ( BMP_ADDR + 1, REG_ID );

	for ( i=0; i<4; i++ ) {
		val = read_id ();
		os_printf ( "BMP180 id = %02x\n", val );
	}

	val = read_idx ();
	os_printf ( "BMP180 id = %02x\n", val );
}

/* test for ARM */
void iic_test ( void )
{
	/* on the BBB,
	 *	sda = gpio_2_2 = P8 pin 7
	 *	scl = gpio_2_5 = P8 pin 9
	 */
	iic_init ( GPIO_2_2, GPIO_2_5 );

	iic_tester ();

	//iic_tester ();

	// iic_pulses ();
}
#endif

#ifdef ARCH_ESP8266
static os_timer_t timer;

static void ICACHE_FLASH_ATTR
ticker ( void *arg )
{
	iic_tester ();
}

#define IIC_SDA		4
#define IIC_SCL		5

/* Reading the BMP180 sensor takes 4.5 + 7.5 milliseconds */
#define DELAY 1000 /* milliseconds */
// #define DELAY 2 /* 2 milliseconds - for DAC test */

void user_init(void)
{
	// Configure the UART
	// uart_init(BIT_RATE_9600,0);
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	os_printf ( "\n" );

	iic_init ( IIC_SDA, IIC_SCL );

	// os_printf ( "BMP180 address: %02x\n", BMP_ADDR );
	// os_printf ( "OSS = %d\n", OSS );

	// Set up a timer to tick continually
	os_timer_disarm(&timer);
	os_timer_setfn(&timer, ticker, (void *)0);
	os_timer_arm(&timer, DELAY, 1);
}
#endif /* ARCH_ESP8266 */

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

/* Glue routines (sort of) to emulate Kyu interface.
 * Mostly taken directly from kyu/bbb/i2c.c
 */

void
i2c_read_reg_n ( struct i2c *ip, int addr, int reg, unsigned char *buf, int n )
{
        buf[0] = reg;

        if ( i2c_send ( ip, addr, buf, 1 ) )
            printf ( "send Trouble\n" );

        if ( i2c_recv ( ip, addr, buf, n ) )
            printf ( "recv Trouble\n" );
}

void
i2c_write_8 ( struct i2c *ip, int addr, int reg, int val )
{
        char buf[2];

        buf[0] = reg;
        buf[1] = val;

        if ( i2c_send ( ip, addr, buf, 2 ) )
            printf ( "i2c_write, send trouble\n" );
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

        i2c_read_reg_n ( ip, addr, reg, buf, 3 );

        rv =  buf[0] << 16;
        rv |= buf[1] << 8;
        rv |= buf[2];

        return rv;
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

static int
bmp_temp ( struct i2c *ip )
{
	int rv;

	// (void) iic_write ( BMP_ADDR, REG_CONTROL, CMD_TEMP );
	i2c_write_8 ( ip, BMP_ADDR, REG_CONTROL, CMD_TEMP );

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

	// (void) iic_write ( BMP_ADDR, REG_CONTROL, CMD_PRESS );
	i2c_write_8 ( ip, BMP_ADDR, REG_CONTROL, CMD_PRESS );

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

void
bmp_once ( struct i2c *ip )
{
	int t, p;

	t = bmp_temp ( ip );
	p = bmp_pressure ( ip );

	printf ( "Raw T, P = %d (%h)  %d (%h)\n", t, t, p, p );

	//os_printf ( "temp = %d\n", val );
	//os_printf ( "pressure = %d\n", val );
	//os_printf ( "raw T, P = %d  %d\n", t, p );

	convert ( t, p );
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
	    printf ( "BMP temp = %d (%h)\n", tt, tt );
	}

	for ( i=0; i<8; i++ ) {
	    id = bmp_id ( ip );
	    printf ( "BMP ID = %x\n", id );
	    tt = bmp_temp ( ip );
	    printf ( "BMP temp = %d (%h)\n", tt, tt );
	}

	pp = bmp_pressure ( ip );
	printf ( "BMP pressure = %d (%h)\n", pp, pp );

	bmp_cals ( ip, (unsigned short *) &bmp_cal );
	show_cals ( (void *) &bmp_cal );

	// for ( i=0; i<8; i++ ) {
	for ( ;; ) {
	    bmp_once ( ip );
	    delay ( 2000 );
	}
}

void
bmp_init ( struct i2c *aip )
{
	bmp_cals ( aip, (unsigned short *) &bmp_cal );
	// bmp_diag1 ( aip );
}

/* THE END */

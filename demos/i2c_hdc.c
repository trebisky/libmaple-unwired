/*
 * Copyright (C) 2020  Tom Trebisky  <tom@mmto.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

/* This is a little "demo driver" for the HDC1008.
 * I have one of these on a little Adafruit board.
 * It uses I2C address 0x40, 0x41, 0x42 or 0x43
 *  (without jumpers, it uses 0x40)
 * This is a Texas Instruments part.
 * It can be run from 3.3 or 5 volts.
 *
 *  HDC1008 temperature and humidity sensor
 */

/* This began as i2c code in the Kyu project.
 * Ported 10-12-2020 to libmaple-unwired.
 *
 * It works (it ran for 3401460 updates on 10-26-2020),
 *  but only with a long delay (40 ms) between i2c send and recv.
 * This doesn't seem right -- I need to look at the HCD1008 datasheet
 * as well as the i2c documents and think about this.
 *
 * From the point of view of just getting T and H data, this is no big deal.
 * Those parameters won't be changing at sub-second times in any environment
 * I can imagine monitoring.
 * If the 40 ms delay is done via busy waiting loops, then that does tie
 *  up the processor in a significant way.
 * This may not matter for many applications, but there is still something
 *  amiss with the protocol that needs investigation.
 */

#define MONSTER_DELAY	40

/* Define this if you want to use a USB serial console.
 * XXX - So far this does weird stuff on a real Maple board.
#define USE_USB_SERIAL
 */

#include <libmaple/util.h>
#include <libmaple/i2c.h>

#include <serial.h>
#include <delay.h>
#include <time.h>

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

/* totally bogus to simulate Kyu API,
 *  --- we will clean up later.
 */
struct i2c {
    int bogus;
};

static void hdc_once ( struct i2c *, int );

void
hdc_test ( void )
{
	struct i2c *ip;
	int count = 0;

	ip = (struct i2c *) 0;

	for ( ;; ) {
	    // printf ( "************************* ***** ********************\n" );
	    // printf ( "************************* WRITE ********************\n" );
	    hdc_once ( ip, ++count );
	    // delay ( 200 );
	    // delay ( 20 );
	    // spin ();
	}

	// hdc_once ( ip );
	// hdc_con_get ( ip );
	// hdc_serial_get ( ip );
}

void
main(void)
{
    int fd;
    int x;

#ifdef USE_USB_SERIAL
    fd = serial_begin ( SERIAL_USB, 999 );
#else
    fd = serial_begin ( SERIAL_1, 115200 );
#endif
    set_std_serial ( fd );

    i2c_master_enable ( I2C2, 0, 100000 );
    i2c_set_debug ( 99 );

    printf ( "-- BOOTED -- \n" );

#ifdef USE_USB_SERIAL
    printf ( "Waiting for you to type something\n" );
    x = getc ();
#endif

    printf ( "-- OK -- off we go\n" );

    hdc_test ();
}

#define I2C_TIMEOUT	3000

#ifdef notdef
/* These are in i2c.h */
#define I2C_MSG_READ            0x1
#define I2C_MSG_10BIT_ADDR      0x2
#endif

void
i2c_trouble ( char *msg, int status )
{
	printf ( "Finished %s: returned: %d -- ", msg, status );
	if ( status == I2C_ERROR_TIMEOUT )
	    printf ( "Timeout\n" );
	else if ( status == I2C_ERROR_PROTOCOL )
	    printf ( "Protocol\n" );
	else
	    printf ( "Unknown error\n" );
	printf ( "Trouble with i2c, spinning\n" );
	spin ();
}

/* Also bogus for now.
 * once things are working, absorb this into lcd_write()
 * This is a kyu API.
 */
void
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
    // printf ( "Finished i2c_send: returned: %d\n", stat );
    if ( stat )
	i2c_trouble ( "i2c_send", stat );
}

void
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
    // printf ( "Finished i2c_recv: returned: %d\n", stat );
    if ( stat )
	i2c_trouble ( "i2c_recv", stat );
}

/* Code below here is almost unchanged from Kyu
 */

/* ---------------------------------------------- */
/* ---------------------------------------------- */

/* HDC1008 driver
 */

#define HDC_ADDR	0x40

/* These are the registers in the device */
#define HDC_TEMP	0x00
#define HDC_HUM		0x01
#define HDC_CON		0x02

#define HDC_SN1		0xFB
#define HDC_SN2		0xFC
#define HDC_SN3		0xFD

/* bits/fields in config register */
#define HDC_RESET	0x8000
#define HDC_HEAT	0x2000
#define HDC_BOTH	0x1000
#define HDC_BSTAT	0x0800

#define HDC_TRES14	0x0000
#define HDC_TRES11	0x0400

#define HDC_HRES14	0x0000
#define HDC_HRES11	0x0100
#define HDC_HRES8	0x0200

/* Delays in microseconds */
#define CONV_8		2500
#define CONV_11		3650
#define CONV_14		6350

#define CONV_BOTH	12700

/* We either read or write some register on the device.
 * to write, there is one i2c transaction
 *  we send 3 bytes (one is the register, then next two the value)
 * to read, there are two i2c transations
 *  The first sends one byte (the register number)
 *  The second reads two bytes (the register value)
 */

/* Write to the config register -- tell it to do both conversions */
static void
hdc_con_both ( struct i2c *ip )
{
	unsigned char buf[3];

	//(void) iic_write16 ( HDC_ADDR, HDC_CON, HDC_BOTH );
	buf[0] = HDC_CON;
	buf[1] = HDC_BOTH >> 8;
	buf[2] = HDC_BOTH & 0xff;
	i2c_send ( ip, HDC_ADDR, buf, 3 );

	/* unfinished */
}

/* Read gets either T or H */
static void
hdc_con_single ( struct i2c *ip )
{
	unsigned char buf[3];

	// (void) iic_write16 ( HDC_ADDR, HDC_CON, 0 );
	buf[0] = HDC_CON;
	buf[1] = 0;
	buf[2] = 0;
	i2c_send ( ip, HDC_ADDR, buf, 3 );

	/* unfinished */
}

/* Get the contents of the config register
 * After reset, the value seems to be 0x1000
 *  which is HDC_BOTH
 */
static void
hdc_con_get ( struct i2c *ip )
{
	unsigned char buf[2];
	int val;

	buf[0] = HDC_CON;
	i2c_send ( ip, HDC_ADDR, buf, 1 );

	i2c_recv ( ip, HDC_ADDR, buf, 2 );
	val = buf[0] << 8 | buf[1];

	printf ( "HDC1008 config = %04x\n", val );
}

#ifdef notdef
/* Let's see if this autoincrements across 3 registers
 * it does not:
 * I get: 023c ffff ffff
 */
static void
hdc_xserial_get ( struct i2c *ip )
{
	unsigned char buf[6];
	int s1, s2, s3;

	buf[0] = HDC_SN1;
	i2c_send ( ip, HDC_ADDR, buf, 1 );

	i2c_recv ( ip, HDC_ADDR, buf, 6 );

	s1 = buf[0] << 8 | buf[1];
	s2 = buf[2] << 8 | buf[3];
	s3 = buf[4] << 8 | buf[5];

	printf ( "HDC1008 X serial = %04x %04x %04x\n", s1, s2, s3 );
}
#endif

/* Get the 3 bytes of serial information.
 * I get: 023c ecab ee00
 */
static void
hdc_serial_get ( struct i2c *ip )
{
	unsigned char buf[2];
	int s1, s2, s3;

	buf[0] = HDC_SN1;
	i2c_send ( ip, HDC_ADDR, buf, 1 );
	i2c_recv ( ip, HDC_ADDR, buf, 6 );
	s1 = buf[0] << 8 | buf[1];

	buf[0] = HDC_SN2;
	i2c_send ( ip, HDC_ADDR, buf, 1 );
	i2c_recv ( ip, HDC_ADDR, buf, 6 );
	s2 = buf[0] << 8 | buf[1];

	buf[0] = HDC_SN3;
	i2c_send ( ip, HDC_ADDR, buf, 1 );
	i2c_recv ( ip, HDC_ADDR, buf, 6 );
	s3 = buf[0] << 8 | buf[1];

	printf ( "HDC1008 serial = %04x %04x %04x\n", s1, s2, s3 );
}

/* Read both T and H */
/* Here we write the register pointer to 0 (HDC_TEMP)
 * then read 4 bytes (two registers).  This back to back read
 * is described in the datasheet.  Apparently the internal
 * register pointer increments automatically to the next
 * register.  Perhaps we could read 6 bytes and also read
 * the config register, which comes next.
 *
 * The device seems to power up in HDC_BOTH mode.
 */

static void
hdc_read_both ( struct i2c *ip, unsigned int *buf )
{
	unsigned char bbuf[4];

	// printf ( "begin read both\n" );
	bbuf[0] = HDC_TEMP;
	i2c_send ( ip, HDC_ADDR, bbuf, 1 );

	// printf ( "MID read both\n" );
	// delay ( 5 ); nope
	// delay ( 10 ); nope
	// delay ( 20 ); nope
	// delay ( 25 ); nope
	// delay ( 50 ); OK
	// delay ( 40 ); OK
	delay ( MONSTER_DELAY );

	// Worked on the Orange Pi
	// delay_us ( CONV_BOTH );

	// XXX
	// i2c_set_debug ( 99 );

	i2c_recv ( ip, HDC_ADDR, bbuf, 4 );
	// printf ( "end read both\n" );

	buf[0] = bbuf[0] << 8 | bbuf[1];
	buf[1] = bbuf[2] << 8 | bbuf[3];
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

static void
hdc_once ( struct i2c *ip, int count )
{
	unsigned int iobuf[2];
	int t, h;
	int tf;

	hdc_read_both ( ip, iobuf );

	/*
	t = 99;
	h = 23;
	tf = 0;
	printf ( " Bogus t, tf, h = %d  %d %d\n", t, tf, h );
	*/
	// printf ( " HDC raw t,h = %04x  %04x\n", iobuf[0], iobuf[1] );

	t = ((iobuf[0] * 165) / 65536)- 40;
	tf = t * 18 / 10 + 32;

	// printf ( " -- traw, t, tf = %04x %d   %d %d\n", iobuf[0], iobuf[0], t, tf );

	//h = ((iobuf[1] * 100) / 65536);
	//printf ( "HDC t, tf, h = %d  %d %d\n", t, tf, h );

	h = ((iobuf[1] * 100) / 65536);
	// printf ( " -- hraw, h = %04x %d    %d\n", iobuf[1], iobuf[1], h );

	// printf ( "HDC temp(F), humid = %d %d\n", tf, h );

	printf ( "HDC temp(F), humid = %d %d (count = %d)\n", tf, h, count );
}

#ifdef notdef
/* Kyu code */
#define I2C_PORT	0

void
hdc_test ( void )
{
	struct i2c *ip;

	ip = i2c_hw_new ( I2C_PORT );
	ip = (struct i2c *) 0;

	thr_new_repeat ( "hdc", hdc_once, ip, 13, 0, 1000 );

	// hdc_once ( ip );
	// hdc_con_get ( ip );
	// hdc_serial_get ( ip );
}
#endif

/* THE END */

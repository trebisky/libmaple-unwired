/*
 * Copyright (C) 2016  Tom Trebisky  <tom@mmto.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

/* bmp390 i2c "driver" for the STM32F103 (maple-unwired).
 * Tom Trebisky  5-21-2021
 */

/* Hardware i2c on the stm32 is buggy and erratic, so we use
 * "software" i2c (i.e. a bit-bang low level driver).
 *
 * Here we have the Bosch BMP390 pressure and temperature sensor.
 *
 * I had hoped that it was just a mild change from the BMP180,
 * but it is quite different and requires its own driver.
 *
 * I grounded the pin to give the alternate i2c address of 0x76
 * so it could share the i2c bus with my bmp180 at address 0x77
 *
 * It returns a device ID of 0x60
 */

/* For maple-unwired */
#include <unwired.h>
#include <i2c.h>

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

/* Glue routines (sort of) to emulate Kyu interface.
 * Mostly taken directly from kyu/bbb/i2c.c
 */

static void
i2c_read_reg_n ( struct i2c *ip, int addr, int reg, unsigned char *buf, int n )
{
        buf[0] = reg;

        if ( i2c_send ( ip, addr, buf, 1 ) )
            printf ( "send Trouble\n" );

        if ( i2c_recv ( ip, addr, buf, n ) )
            printf ( "recv Trouble\n" );
}

static void
i2c_write_8 ( struct i2c *ip, int addr, int reg, int val )
{
        char buf[2];

        buf[0] = reg;
        buf[1] = val;

        if ( i2c_send ( ip, addr, buf, 2 ) )
            printf ( "i2c_write, send trouble\n" );
}

static int
i2c_read_8 ( struct i2c *ip, int addr, int reg )
{
        char buf[1];

        i2c_read_reg_n ( ip, addr, reg, buf, 1 );

        return buf[0] & 0xff;
}

static int
i2c_read_16 ( struct i2c *ip, int addr, int reg )
{
        int rv;
        unsigned char buf[2];

        i2c_read_reg_n ( ip, addr, reg, buf, 2 );

        rv = buf[0] << 8;
        rv |= buf[1];

        return rv;
}

static int
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

#define BMPX_ADDR	0x76
#define REG_ID_X	0

/* Returns 0x60 (the bmp180 gave 0x55)
 */
static int
bmpx_id ( struct i2c *ip )
{
	return i2c_read_8 ( ip, BMPX_ADDR, REG_ID_X );
}


void
bmpx_diag ( struct i2c *ip )
{
	int id;
	int tt, pp;
	unsigned short cals[11];
	int i;

	for ( i=0; i<8; i++ ) {
	    id = bmpx_id ( ip );
	    printf ( "BMPX ID = %x\n", id );
	}


#ifdef notyet
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
#endif
}

void
bmpx_init ( struct i2c *aip )
{
	bmpx_diag ( aip );
}

/* THE END */

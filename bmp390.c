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
        unsigned char buf[1];

        i2c_read_reg_n ( ip, addr, reg, buf, 1 );

        return buf[0] & 0xff;
}

/* We want a signed 8 bit value */
static int
i2c_read_8s ( struct i2c *ip, int addr, int reg )
{
        signed char buf[1];

        i2c_read_reg_n ( ip, addr, reg, buf, 1 );

        return (int) buf[0];
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

/* little endian version of the above.
 * returns unsigned short
 */
static int
i2c_read_16_le ( struct i2c *ip, int addr, int reg )
{
        int rv;
        unsigned char buf[2];

        i2c_read_reg_n ( ip, addr, reg, buf, 2 );

        rv = buf[1] << 8;
        rv |= buf[0];

        return rv;
}

/* little endian, returns signed short */
static int
i2c_read_16_les ( struct i2c *ip, int addr, int reg )
{
        int rv;
        unsigned char buf[2];
	short val;

        i2c_read_reg_n ( ip, addr, reg, buf, 2 );

        val = buf[1] << 8;
        val |= buf[0];
	rv = val;

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

static int
i2c_read_24_le ( struct i2c *ip, int addr, int reg )
{
        int rv;
        unsigned char buf[3];

        i2c_read_reg_n ( ip, addr, reg, buf, 3 );

        rv =  buf[2] << 16;
        rv |= buf[1] << 8;
        rv |= buf[0];

        return rv;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

#define BMPX_ADDR	0x76
#define REG_ID		0
#define REG_REV		1
#define REG_ERR		2
#define REG_STAT	3
#define REG_PRESS	4
#define REG_TEMP	7
#define REG_PWR		0x1b

#define REG_CAL_T1	0x31
#define REG_CAL_T2	0x33
#define REG_CAL_T3	0x35

#define REG_CAL_P1	0x36
#define REG_CAL_P2	0x38
#define REG_CAL_P3	0x3A
#define REG_CAL_P4	0x3B
#define REG_CAL_P5	0x3C
#define REG_CAL_P6	0x3E
#define REG_CAL_P7	0x40
#define REG_CAL_P8	0x41
#define REG_CAL_P9	0x42
#define REG_CAL_P10	0x44
#define REG_CAL_P11	0x45

#define CMD_CMD		0x10	/* ready for command */
#define CMD_P		0x20	/* P ready for readout */
#define CMD_T		0x40	/* T ready for readout */

#define PWR_P_ENA	0x01
#define PWR_T_ENA	0x02
#define PWR_SLEEP	0x00
#define PWR_FORCE	0x10
#define PWR_FORCE2	0x20
#define PWR_NORM	0x30

#define FORCE		( PWR_P_ENA | PWR_T_ENA | PWR_FORCE )

/* Note that with the single lone char for t3, we must
 * give gcc the "packed" attribute or we get an evil pad byte.
 */

struct bmp_cal_raw {
	unsigned short t1;
	unsigned short t2;
	signed char t3;

	short p1;
	short p2;
	signed char p3;
	signed char p4;
	unsigned short p5;
	unsigned short p6;
	signed char p7;
	signed char p8;
	short p9;
	signed char p10;
	signed char p11;
} __attribute__((packed));

struct bmp_cal {
	int t1;
	int t2;
	int t3;

	int p1;
	int p2;
	int p3;
	int p4;
	int p5;
	int p6;
	int p7;
	int p8;
	int p9;
	int p10;
	int p11;
};

static struct bmp_cal cals;

/* Returns 0x60 (the bmp180 gave 0x55)
 */
static int
bmpx_id ( struct i2c *ip )
{
	return i2c_read_8 ( ip, BMPX_ADDR, REG_ID );
}

static int
bmpx_rev ( struct i2c *ip )
{
	return i2c_read_8 ( ip, BMPX_ADDR, REG_REV );
}

static int
bmpx_err ( struct i2c *ip )
{
	return i2c_read_8 ( ip, BMPX_ADDR, REG_ERR );
}

static int
bmpx_stat ( struct i2c *ip )
{
	return i2c_read_8 ( ip, BMPX_ADDR, REG_STAT );
}

static void
bmpx_force ( struct i2c *ip )
{
	i2c_write_8 ( ip, BMPX_ADDR, REG_PWR, FORCE );
}

static int
bmpx_temp ( struct i2c *ip )
{
	return i2c_read_24_le ( ip, BMPX_ADDR, REG_TEMP );
}

static int
bmpx_press ( struct i2c *ip )
{
	return i2c_read_24_le ( ip, BMPX_ADDR, REG_PRESS );
}

static void
bmpx_show ( struct i2c *ip )
{
	int err, stat;

	err = bmpx_err ( ip );
	stat = bmpx_stat ( ip );
	printf ( " err, stat = %h %h\n", err, stat );
}

#define CAL_SIZE	21

/* Amazingly enough, the device handles this just fine,
 * reading out all the calibration registers in a single
 * transaction.
 */
static void
bmpx_read_cal_data ( struct i2c *ip )
{
	unsigned char cal_buf[32];
	struct bmp_cal_raw raw;

        i2c_read_reg_n ( ip, BMPX_ADDR, REG_CAL_T1, cal_buf, CAL_SIZE );

	printf ( "Raw cal struct is %d\n", sizeof(struct bmp_cal_raw) );
	if ( sizeof(struct bmp_cal_raw) != 21 )
	    printf ( "Bad compiler jive, dude!\n" );
	printf ( "T1 is at - %h\n", &raw.t1 );
	printf ( "P11 is at - %h\n", &raw.p11 );

}

static void
bmpx_read_cals ( struct i2c *ip )
{
	cals.t1 = i2c_read_16_le ( ip, BMPX_ADDR, REG_CAL_T1 );
	cals.t2 = i2c_read_16_le ( ip, BMPX_ADDR, REG_CAL_T2 );
	cals.t3 = i2c_read_8s ( ip, BMPX_ADDR, REG_CAL_T3 );

	cals.p1 = i2c_read_16_les ( ip, BMPX_ADDR, REG_CAL_P1 );
	cals.p2 = i2c_read_16_les ( ip, BMPX_ADDR, REG_CAL_P2 );
	cals.p3 = i2c_read_8s ( ip, BMPX_ADDR, REG_CAL_P3 );
	cals.p4 = i2c_read_8s ( ip, BMPX_ADDR, REG_CAL_P4 );
	cals.p5 = i2c_read_16_le ( ip, BMPX_ADDR, REG_CAL_P5 );
	cals.p6 = i2c_read_16_le ( ip, BMPX_ADDR, REG_CAL_P6 );
	cals.p7 = i2c_read_8s ( ip, BMPX_ADDR, REG_CAL_P7 );
	cals.p8 = i2c_read_8s ( ip, BMPX_ADDR, REG_CAL_P8 );
	cals.p9 = i2c_read_16_les ( ip, BMPX_ADDR, REG_CAL_P9 );
	cals.p10 = i2c_read_8s ( ip, BMPX_ADDR, REG_CAL_P10 );
	cals.p11 = i2c_read_8s ( ip, BMPX_ADDR, REG_CAL_P11 );
}

static void
bmpx_print_cal ( char *name, int val )
{
	printf ( "Cal %s = %d (%h)\n", name, val, val );
}

static void
bmpx_show_cals ( void )
{
	bmpx_print_cal ( "T1", cals.t1 );
	bmpx_print_cal ( "T2", cals.t2 );
	bmpx_print_cal ( "T3", cals.t3 );

	bmpx_print_cal ( "P1", cals.p1 );
	bmpx_print_cal ( "P2", cals.p2 );
	bmpx_print_cal ( "P3", cals.p3 );
	bmpx_print_cal ( "P4", cals.p4 );
	bmpx_print_cal ( "P5", cals.p5 );
	bmpx_print_cal ( "P6", cals.p6 );
	bmpx_print_cal ( "P7", cals.p7 );
	bmpx_print_cal ( "P8", cals.p8 );
	bmpx_print_cal ( "P9", cals.p9 );
	bmpx_print_cal ( "P10", cals.p10 );
	bmpx_print_cal ( "P11", cals.p11 );
}

typedef unsigned long long u64;
typedef long long i64;

static void
show64 ( char *msg, i64 aval )
{
	int val = aval;

	printf ( " %s = %d (%h)\n", msg, val, val );
}

static void
show64u ( char *msg, u64 aval )
{
	int val = aval;

	printf ( " %s = %d (%h)\n", msg, val, val );
}

/* return temperature in degrees*100 */

i64 t_lin;

static int
convert_temp ( int traw )
{
	u64 d1, d2, d3;
	i64 d4, d5, d6;
	i64 rv;

	d1 = traw - 256 * cals.t1;
	// show64u ( "d1", d1 );

	d2 = cals.t2 * d1;
	// show64u ( "d2", d2 );
	// show64u ( "d2<<18", d2<<18 );

	d3 = d1 * d1;
	// show64u ( "d3", d3 );

	d4 = d3 * cals.t3;
	// show64 ( "d4", d4 );

	// gotta have these parens.
	d5 = (d2 << 18) + d4;
	d6 = d5 >> 32;
	// show64 ( "d6", d6 );
	t_lin = d6 >> 16;

	rv = d6 * 100;
	return rv >> 16;
}

static int
convert_pressure ( int praw )
{
	i64 d1;
	i64 out1, out2, out3;
	i64 tt, ttt;
	i64 pp;
	i64 rv;
	int x;

	tt = t_lin * t_lin;
	ttt = tt * t_lin;

	out1 = cals.p5 << 18;
	out1 += (cals.p6 * t_lin) << 9;
	out1 += (cals.p7 * tt) << 7;
	out1 += cals.p8 * ttt;
	out1 >>= 15;
	/* 31031 */

	out2 = (cals.p1 - 16384) * 131072;
	out2 += (cals.p2 - 16384) * 256 * t_lin;
	out2 += cals.p3 * 32 * tt;
	out2 += cals.p4 * ttt;
	out2 *= praw;
	// shifting don't work if negative.
	// out2 >>= 37;
	out2 /= 137438953472;
	x = out2;
	printf ( " out2 = %d\n", x );

	pp = praw * praw;
	out3 = cals.p9 + cals.p10 * t_lin;
	out3 *= pp;
	d1 = cals.p11 * praw * pp;
	d1 >>= 17;
	out3 += d1;
	out3 >>= 48;
	x = out3;
	printf ( " out3 = %d\n", x );
	out3 = 0;

	rv = out1 + out2 + out3;
	return rv;
}

#ifdef notdef
static int
convert_pressure ( int praw )
{
	i64 d1, d2, d3;
	i64 d4, d5, d6;
	i64 off, sens;
	i64 rv;

	d1 = f_temp * f_temp;
	d2 = d1 / 64;
	d3 = (d2 * f_temp) / 256;
	d4 = (cals.p8 * d3) / 32;
	d5 = cals.p7 * d1 * 16;
	d6 = cals.p6 * f_temp * 4194304;
	off = cals.p5 * 140737488355328 + d4 + d5 + d6;
	d2 = cals.p4 * d3 / 32;
	d4 = cals.p3 * d1 * 4;
	d5 = (cals.p2 - 16384) * f_temp * 2097152;

	sens = (cals.p1 - 16384) * 70368744177664 + d2 + d4 + d5;
	d1 = (sens / 16777216) * praw;
	d2 = cals.p10 * f_temp;
	d3 = d2 + 65536 * cals.p9;
	d4 = d3 * praw / 8192;
	d5 = d4 * praw / 512;
	d6 = praw * praw;
	d2 = cals.p11 + d6 / 65536;
	d3 = d2 * praw / 128;

	d4 = off / 4 + d1 + d5 + d3;
	rv = d4 * 25;
	rv /= 1099511627776;

	return rv;
}

static int64_t
_bmp388_compensate_pressure(bmp388_handle_t *handle, uint32_t data)
{
    volatile int64_t partial_data1;
    volatile int64_t partial_data2;
    volatile int64_t partial_data3;
    volatile int64_t partial_data4;
    volatile int64_t partial_data5;
    volatile int64_t partial_data6;
    volatile int64_t offset;
    volatile int64_t sensitivity;
    volatile uint64_t comp_press;

    /* calculate compensate pressure */
    partial_data1 = handle->t_fine * handle->t_fine;
    partial_data2 = partial_data1 / 64;
    partial_data3 = (partial_data2 * handle->t_fine) / 256;
    partial_data4 = (handle->p8 * partial_data3) / 32;
    partial_data5 = (handle->p7 * partial_data1) * 16;
    partial_data6 = (handle->p6 * handle->t_fine) * 4194304;
    offset = (int64_t)((int64_t)(handle->p5) * (int64_t)140737488355328) + partial_data4 + partial_data5 + partial_data6;
    partial_data2 = (((int64_t)handle->p4) * partial_data3) / 32;
    partial_data4 = (handle->p3 * partial_data1) * 4;
    partial_data5 = ((int64_t)(handle->p2) - 16384) * ((int64_t)handle->t_fine) * 2097152;
    sensitivity = (((int64_t)(handle->p1) - 16384) * (int64_t)70368744177664) + partial_data2 + partial_data4 + partial_data5;
    partial_data1 = (sensitivity / 16777216) * data;
    partial_data2 = (int64_t)(handle->p10) * (int64_t)(handle->t_fine);
    partial_data3 = partial_data2 + (65536 * (int64_t)(handle->p9));
    partial_data4 = (partial_data3 * data) / 8192;
    partial_data5 = (partial_data4 * data) / 512;
    partial_data6 = (int64_t)((uint64_t)data * (uint64_t)data);
    partial_data2 = ((int64_t)(handle->p11) * (int64_t)(partial_data6)) / 65536;
    partial_data3 = (partial_data2 * data) / 128;
    partial_data4 = (offset / 4) + partial_data1 + partial_data5 + partial_data3;
    comp_press = (((uint64_t)partial_data4 * 25) / (uint64_t)1099511627776);

    return comp_press;
}
#endif

void
bmpx_diag ( struct i2c *ip )
{
	int id;
	int tt, pp;
	int tc, tf;
	int p;
	int i;

#ifdef notdef
	for ( i=0; i<2; i++ ) {
	    id = bmpx_id ( ip );
	    printf ( "BMPX ID = %x\n", id );
	    id = bmpx_rev ( ip );
	    printf ( "BMPX REV = %x\n", id );
	}

	bmpx_read_cals ( ip );
	bmpx_show_cals ();
#endif

	for ( i=0; i<1; i++ ) {
	    bmpx_force ( ip );
	    // bmpx_show ( ip );
	    delay ( 200 );
	    // bmpx_show ( ip );

	    // pp = bmpx_press ( ip );
	    // printf ( "BMP pressure = %d (%h)\n", pp, pp );
	    tt = bmpx_temp ( ip );
	    printf ( "BMP raw temp = %d (%h)\n", tt, tt );
	    tc = convert_temp ( tt );
	    printf ( "BMP temp (C*100) = %d\n", tc );
	    tf = tc * 18;
	    tf = 3200 + tf / 10;
	    printf ( "BMP temp (F*100) = %d\n", tf );

	    pp = bmpx_press ( ip );
	    printf ( "BMP raw pressure = %d (%h)\n", pp, pp );
	    p = convert_pressure ( pp );
	    printf ( "BMP pressure () = %d\n", p );
	}

#ifdef notyet
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
bmpx_init ( struct i2c *ip )
{
	int id;

	id = bmpx_id ( ip );
	printf ( "BMPX ID = %x\n", id );
	id = bmpx_rev ( ip );
	printf ( "BMPX REV = %x\n", id );

	/* old way */
	bmpx_read_cals ( ip );
	bmpx_show_cals ();

	/* new way */
	bmpx_read_cal_data ( ip );

	bmpx_diag ( ip );
}

/* THE END */

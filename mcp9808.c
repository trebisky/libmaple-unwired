/*
 * Copyright (C) 2016  Tom Trebisky  <tom@mmto.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

/* mcp9808 "driver"
 * Tom Trebisky  5-26-2021
 */

/* Hardware i2c on the stm32 is buggy and erratic, so we use
 * "software" i2c (i.e. a bit-bang low level driver).
 *
 * Here we have the Microchip high precision
 *  temperature sensor.
 * They claim 0.5 degree accuracy and even better resolution
 */

/* For maple-unwired */
#include <unwired.h>
#include <i2c.h>

static int bmp_temp ( struct i2c * );
static int bmp_pressure ( struct i2c * );

/* 3 address bits are provided that can be jumpered.
 * The Adafruit breakout has pulldowns on these, so if
 * you don't pull any up, you get this address.
 */
#define  MCP_ADDR		0x18

/* The MCP9808 has a number of 16 bit registers.
 * They must each be accessed individually, there is
 * no register sequencing logic
 */
#define  REG_CONF		1
#define  REG_TUPPER		2
#define  REG_LOWER		3
#define  REG_CRIT		4
#define  REG_TEMP		5
#define  REG_ID			6
#define  REG_REV		7
#define  REG_RES		8

static int
mcp_read_reg ( struct i2c *ip, int reg )
{
        unsigned char buf[2];

        buf[0] = reg;
        i2c_send ( ip, MCP_ADDR, buf, 1 );
        i2c_recv ( ip, MCP_ADDR, buf, 2 );

        return buf[0]<<8 | buf[1];
}

/* It looks to me like the ID should read as 0x54
 *  -- and indeed it does.
 * The rev reads 0x400
 */

static int
mcp_id ( struct i2c *ip )
{
	return mcp_read_reg ( ip, REG_ID );
}

static int
mcp_rev ( struct i2c *ip )
{
	return mcp_read_reg ( ip, REG_REV );
}

static int
mcp_temp ( struct i2c *ip )
{
	return mcp_read_reg ( ip, REG_TEMP );
}

static int
mcp_show_temp ( struct i2c *ip )
{
	int raw;
	int tc;
	int tf;

	raw = mcp_temp ( ip );
	printf ( "MCP Raw TEMP = %h\n", raw );
	tc = raw & 0xfff;
	tc *= 100;
	tc >>= 4;
	printf ( "MCP Tc = %d\n", tc );
	tf = tc * 18;
	tf = 3200 + tf / 10;
	printf ( "MCP Tf = %d\n", tf );
}

void
mcp_diag ( struct i2c *ip )
{
	printf ( "MCP ID  = %h\n", mcp_id(ip) );
	printf ( "MCP REV = %h\n", mcp_rev(ip) );
	printf ( "MCP TEMP = %h\n", mcp_temp(ip) );
	mcp_show_temp ( ip );
}

void
mcp_init ( struct i2c *ip )
{
	mcp_diag ( ip );
}

/* THE END */

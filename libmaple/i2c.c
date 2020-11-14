/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2020 Tom Trebisky
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/* Libmaple used to have i2c.c, but I renamed that to i2c_hw.c
 * Then I introduced iic.c alongside of it (my bit-bang driver).
 * The STM32F103 silicon is buggy as hell (at least the i2c section).
 * And after wasting several weeks with it, I just shoved it aside and
 * started using my big-bang driver and life has been daisies and butterflies
 * so far.  This file is a top level "switch" routine that presents the
 * public API and allows a caller to pick between i2c and iic with the
 * intial "new" call and the rest of the code stays the same.
 */

#include "i2c.h"

/* In iic.c */
void iic_init ( int, int );
int iic_send ( int, unsigned char *, int );
int iic_recv ( int, unsigned char *, int );

/* In i2c_hw.c */
void i2c_master_enable ( i2c_dev *, uint32, uint32 );

#define MAX_I2C		4

static struct i2c i2c_softc[MAX_I2C];
static int8	cur_i2c = 0;
static int8	num_iic = 0;

static struct i2c *
i2c_alloc ( void )
{
        // return (struct i2c *) malloc ( sizeof(struct i2c) );
	if ( cur_i2c >= MAX_I2C )
	    return NULL;
	return &i2c_softc[cur_i2c++];
}

extern i2c_dev i2c1;
extern i2c_dev i2c2;

/* If you want to try the buggy silicon, use this */
/* valid arguments are 1 or 2 */
struct i2c *
i2c_hw_new ( int num )
{
        struct i2c *ip;
	struct i2c_dev *dev;

	if ( num < 1 || num > 2 )
	    return NULL;

	if ( num == 1 )
	    dev = &i2c1;
	else
	    dev = &i2c2;

	i2c_master_enable ( dev, 0, 100000 );

#ifdef KYU
        void *hwp;
        hwp = i2c_hw_init ( num );
        if ( ! hwp )
            return (struct i2c *) 0;
#endif

        ip = i2c_alloc ();
	if ( ! ip ) return NULL;

        ip->type = I2C_HW;
        // ip->hw = hwp;
        ip->hw = dev;
        return ip;
}

/* If you want the reliable 100 kHz big bang driver, use this */
/*  At present we only allow one of these.
 *   someday if I need more than one, it won't be hard to manage.
 */
struct i2c *
i2c_gpio_new ( int sda_pin, int scl_pin )
{
        struct i2c *ip;

	/* Enforce "only one of these" */
	if ( num_iic ) return NULL;
	num_iic++;

        iic_init ( sda_pin, scl_pin );

        ip = i2c_alloc ();
	if ( ! ip ) return NULL;

        ip->type = I2C_GPIO;
        return ip;
}

/* Send and Receive are the fundamental i2c primitives
 * that everything else gets routed through.
 * Return 0 if OK, 1 if trouble (usually a NACK).
 *
 * XXX - HW support is not yet done.
 */

int
i2c_send ( struct i2c *ip, int addr, char *buf, int count )
{
        if ( ip->type == I2C_HW ) {
            //return i2c_hw_send ( ip->hw, addr, buf, count );
	    return 1;
        } else {
            return iic_send ( addr, buf, count );
        }
}

int
i2c_recv ( struct i2c *ip, int addr, char *buf, int count )
{
        if ( ip->type == I2C_HW ) {
            // return i2c_hw_recv ( ip->hw, addr, buf, count );
	    return 1;
        } else {
            return iic_recv ( addr, buf, count );
        }
}

/* THE END */

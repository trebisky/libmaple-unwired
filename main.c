// hacked for "unwired" setup.  10-10-2020
// And it "just worked" the first time.
// That almost never happens.
//
// On 10-21-2020 I ran it overnight with no problems,
// OK - wait for 3 -> 2 (0) 11357820
// over 11 million transactions.
// Note that hitting the reset button at the right
// moment leaves the i2c bus hung with no recovery
// other than removing power.
//
// i2c-mcp4725-dac.cpp
//
// Written by Andrew Meyer <ajm@leaflabs.com>
// Modified by Marti Bolivar <mbolivar@leaflabs.com>
//
// Simple program showing how to control an MCP4725 DAC using the
// libmaple I2C interface. There's an MCP4725 breakout board available
// on SparkFun:
//
// http://www.sparkfun.com/products/8736
//
// How to use:
//
// 1. Connect the DAC SDA and SCL pins to I2C2, with a pullup
//    resistor (1 KOhm should work) to VCC.
// 2. Load the sketch and connect to SerialUSB.
//	(I use Serial1)
// 3. Press the button.
//	(forget it, we ain't got no button)
//
// The program then makes sure the DAC is connected properly (during
// setup()), then has the DAC output a sawtooth wave (with loop()).

// #include <wirish/wirish.h>
// #include <unwired.h>

/* Orange Pi sees this from the MCP4725:
 * DAC status = 18
 * DAC val = c0 0d
 * DAC ee = 40 b8
 */

#include <string.h>

#include <serial.h>
#include <time.h>
#include <boards.h>
#include <i2c.h>

/* Use bing-bang iic driver */
#define USE_IIC

#ifdef USE_IIC
#include <libmaple/i2c.h>
void iic_init ( int, int );
void iic_recv ( int, char *, int );
void iic_send ( int, char *, int );
#else
#include <libmaple/i2c.h>
#endif

#define MCP_ADDR         0x60

#define MCP_WRITE_DAC    0b01000000
#define MCP_WRITE_EEPROM 0b01100000
#define MCP_PD_NORMAL    0b00000000
#define MCP_PD_1K        0b00000010
#define MCP_PD_100K      0b00000100
#define MCP_PD_500K      0b00000110

static uint8 write_msg_data[3];
static i2c_msg write_msg;

static uint8 read_msg_data[5];
static i2c_msg read_msg;

#ifdef notdef
/*
 * DAC control routines
 */

void 
mcp_i2c_setup (void)
{
    write_msg.addr = MCP_ADDR;
    write_msg.flags = 0; // write, 7 bit address
    write_msg.length = sizeof(write_msg_data);
    write_msg.xferred = 0;
    write_msg.data = write_msg_data;

    read_msg.addr = MCP_ADDR;
    read_msg.flags = I2C_MSG_READ;
    read_msg.length = sizeof(read_msg_data);
    read_msg.xferred = 0;
    read_msg.data = read_msg_data;
}

/* This sends 3 bytes */

/* OLD */
void
mcp_write_val
(uint16 val)
{
    int st;

    write_msg_data[0] = MCP_WRITE_DAC | MCP_PD_NORMAL;
    uint16 tmp = val >> 4;
    write_msg_data[1] = tmp;
    tmp = (val << 4) & 0x00FF;
    write_msg_data[2] = tmp;

    st = i2c_master_xfer(I2C2, &write_msg, 1, 0);
    // printf ( "MCP write val Xfer returns: %d\n", st );
}

/* OLD */
void
mcp_dump ( char *msg, uint8 buf[], int n )
{
	int i;

	for ( i=0; i<n; i++ ) {
	    printf ( "%s %d = %h\n", msg, i, buf[i] );
	}
}

/* The read sends one byte (the 0x60 address with the 1 bit set
 *  to indicate a read.  The device responds with 5 bytes:
 *  0) status
 *  1) DAC data (high 8)
 *  2) DAC data (low 4)
 *  3) EEprom data (high 4)
 *  4) EEprom data (low 8)

 */
/* OLD */
uint16
mcp_read_val( void )
{
    uint16 tmp = 0;
    int st;

    st = i2c_master_xfer(I2C2, &read_msg, 1, 2);
    // printf ( "MCP read val Xfer returns: %d\n", st );
    // mcp_dump ( "MCP read",read_msg_data, 5 );

    /* We don't care about the status and EEPROM bytes (0, 3, and 4). */
    tmp = (read_msg_data[1] << 4);
    tmp += (read_msg_data[2] >> 4);
    return tmp;
}
#endif

void 
mcp_i2c_setup (void)
{
}

/* This sends 3 bytes */

#ifndef USE_IIC
void
mcp_write_val
(uint16 val)
{
    int st;

    write_msg.addr = MCP_ADDR;
    write_msg.flags = 0; // write, 7 bit address
    write_msg.length = sizeof(write_msg_data);
    write_msg.xferred = 0;
    write_msg.data = write_msg_data;

    write_msg_data[0] = MCP_WRITE_DAC | MCP_PD_NORMAL;
    uint16 tmp = val >> 4;
    write_msg_data[1] = tmp;
    tmp = (val << 4) & 0x00FF;
    write_msg_data[2] = tmp;

    st = i2c_master_xfer(I2C2, &write_msg, 1, 0);
    // printf ( "MCP write val Xfer returns: %d\n", st );
}
#endif

void
mcp_dump ( char *msg, uint8 buf[], int n )
{
	int i;

	for ( i=0; i<n; i++ ) {
	    printf ( "%s %d = %h\n", msg, i, buf[i] );
	}
}

/* The read sends one byte (the 0x60 address with the 1 bit set
 *  to indicate a read.  The device responds with 5 bytes:
 *  0) status
 *  1) DAC data (high 8)
 *  2) DAC data (low 4)
 *  3) EEprom data (high 4)
 *  4) EEprom data (low 8)
 */

#ifndef USE_IIC
uint16
mcp_read_val( void )
{
    uint16 tmp = 0;
    int st;

    read_msg.addr = MCP_ADDR;
    read_msg.flags = I2C_MSG_READ;
    read_msg.length = sizeof(read_msg_data);
    read_msg.xferred = 0;
    read_msg.data = read_msg_data;

    st = i2c_master_xfer(I2C2, &read_msg, 1, 2);
    // printf ( "MCP read val Xfer returns: %d\n", st );
    // mcp_dump ( "MCP read",read_msg_data, 5 );

    /* We don't care about the status and EEPROM bytes (0, 3, and 4). */
    tmp = (read_msg_data[1] << 4);
    tmp += (read_msg_data[2] >> 4);
    return tmp;
}
#endif

#ifdef USE_IIC

struct i2c *xp;

int
mcp_read_val( void )
{
    int tmp = 0;
    char io_buf[5];

    i2c_recv ( xp, MCP_ADDR, io_buf, 5 );

    // mcp_dump ( "MCP read", io_buf, 5 );

    /* We don't care about the status and EEPROM bytes (0, 3, and 4). */
    tmp = (io_buf[1] << 4);
    tmp += (io_buf[2] >> 4);
    return tmp;
}
void
mcp_write_val ( int val )
{
    unsigned char io_buf[2];

    io_buf[0] = (val >> 8) & 0xf;
    io_buf[1] = val & 0xff;

    i2c_send ( xp, MCP_ADDR, io_buf, 2 );

    // write_msg_data[0] = MCP_WRITE_DAC | MCP_PD_NORMAL;
    // write_msg_data[0] = MCP_WRITE_DAC | MCP_PD_NORMAL;
    // uint16 tmp = val >> 4;
    // write_msg_data[1] = tmp;
    // tmp = (val << 4) & 0x00FF;
    // write_msg_data[2] = tmp;
}
#endif

/* This is a 12 bit dac, so we can write at most 0x3ff
 */
#define TEST1	0x101
#define TEST2	0x3ab
#define TEST9	0x300

int
mcp_test ( void )
{
    uint16 val;
    uint16 test_val;
    int8 ok = 1;
    int repeat;

    printf ("Testing the MCP4725...\n");

#ifdef notdef
    printf ("Start first read from the DAC ...\n");

    for ( repeat=0; repeat<5; repeat++ ) {
	/* Read the value of the register (should be 0x0800 if factory fresh) */
	printf ( " ******************* READ ********************\n" );
	val = mcp_read_val();
	printf("DAC Register reads as = %h\n", val);
	val = TEST9 + repeat;
	printf ( " ******************* WRITE (%h) ********************\n", val );
	mcp_write_val ( val );
    }
#endif

    // spin ();

    test_val = TEST1;
    mcp_write_val ( test_val );
    printf("Write:  %h to the DAC\n", test_val);
    val = mcp_read_val();
    printf("Read from DAC = %h\n", val);
    if (val != test_val)
	ok = 0;

    test_val = TEST2;
    mcp_write_val ( test_val );
    printf("Write:  %h to the DAC\n", test_val);
    val = mcp_read_val();
    printf("Read from DAC = %h\n", val);
    if (val != test_val)
	ok = 0;

    if ( ! ok ) {
        printf ("ERROR: MCP4725 not responding correctly\n");
        return 0;
    }

    printf("MCP4725 seems to be working\n");
    return 1;
}

#ifdef USE_IIC

/* From kyu orange_pi/i2c_dac.c */
static void
dac_read ( struct i2c *ip, unsigned char *buf, int n )
{
        iic_recv ( MCP_ADDR, buf, n );
}

static void
dac_show ( struct i2c *ip )
{
        unsigned char io[5];

	memset ( io, 0, 5 );
        dac_read ( ip, io, 5 );

        printf ( "DAC status = %x\n", io[0] );
        printf ( "DAC val = %x %x\n", io[1], io[2] );
        printf ( "DAC ee = %x %x\n", io[3], io[4] );
}
#endif

void
setup ( void )
{
    int fd;
    int i;


    fd = serial_begin ( SERIAL_1, 115200 );
    set_std_serial ( fd );

    printf ( "+++++++++++++++++++++++++++++++++++++++\n" );
    printf ( "+++++++++++++++++++++++++++++++++++++++\n" );
    printf ( "+++++++++++++++++++++++++++++++++++++++\n" );
    printf ( "+++++++++++++++++++++++++++++++++++++++\n" );
    printf ( "+++++++++++++++++++++++++++++++++++++++\n" );
    printf ( "Ready to go\n" );

#ifdef USE_IIC
    /* sda, scl */
    // iic_init ( D30, D29 );
    xp = i2c_gpio_new ( D30, D29 );
    if ( ! xp ) {
	printf ( "Cannot set up GPIO iic\n" );
	spin ();
    }

    for ( i=0; i<4; i++ ) {
	dac_show ( xp );
	delay ( 500 );
    }
#else
    // API change when we adopted the STM32duino i2c driver.
    // i2c_master_enable(I2C2, 0);
    i2c_master_enable ( I2C2, 0, 100000 );
    // i2c_set_debug ( 1 );
#endif

    mcp_i2c_setup();

    if ( ! mcp_test() )
	spin ();

    printf ("Starting sawtooth wave\n");
}

/* At present this gives a sawtooth at about 32 Hz.
 * Any printout will add delays and change that timing.
 */
void
main(void)
{
    static uint16 dout = 0;

    setup();

    for ( ;; ) {
	mcp_write_val(dout);

	dout += 50;
	if (dout >= 32768) {
	    dout = 0;
	}
    }
}

/* THE END */

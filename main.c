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
#include <serial.h>

#include <libmaple/i2c.h>

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
    printf ( "MCP write val Xfer returns: %d\n", st );
}

uint16
mcp_read_val( void )
{
    uint16 tmp = 0;
    int st;

    st = i2c_master_xfer(I2C2, &read_msg, 1, 2);
    printf ( "MCP read val Xfer returns: %d\n", st );

    /* We don't care about the status and EEPROM bytes (0, 3, and 4). */
    tmp = (read_msg_data[1] << 4);
    tmp += (read_msg_data[2] >> 4);
    return tmp;
}

/* This is a 12 bit dac, so we can write at most 0x3ff
 */
#define TEST1	0x101
#define TEST2	0x3ab

int
mcp_test ( void )
{
    uint16 val;
    uint16 test_val = 0x0101;
    int ok = 1;

    printf ("Testing the MCP4725...\n");
    printf ("First read from the DAC ...\n");
    /* Read the value of the register (should be 0x0800 if factory fresh) */
    val = mcp_read_val();

    printf("DAC Register = %h\n", val);

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

void setup() {
    int fd;

    fd = serial_begin ( SERIAL_1, 115200 );
    set_std_serial ( fd );
    printf ( "Ready to go\n" );

    // API change when we adopted the STM32duino i2c driver.
    // i2c_master_enable(I2C2, 0);
    i2c_master_enable ( I2C2, 0, 100000 );
    mcp_i2c_setup();

    if ( ! mcp_test() ) {
	printf ( "Spinning\n" );
	for ( ;; )
	    ;
    }

    printf ("Starting sawtooth wave\n");
}

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

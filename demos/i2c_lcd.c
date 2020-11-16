/* This is i2c_lcd.c from Tom's kyu/orange_pi collection
 * It is a driver for the extremely common 16x2 monochrome
 * LCD module, with the i2c piggyback board.
 * 
 * Copyright (C) 2020  Tom Trebisky  <tom@mmto.org>
 * Tom Trebisky  10-11-2020
 *
 * Work began to get this to run with libmaple-unwired 10-11-2020
 */

/* This is a driver for a 16x2 LCD display
 * The ones I have use a PCF8574 i2c chip.
 * The display itself is based on a Hitachi HD44780.
 * These gadgets are cheap, simple, and well known.
 *
 * It seems that you MUST run this from 5 volts and use a level shifter.
 * This isn't quite true.  The display itself will work from 3.3 volts,
 *  but is very faint (and you probably will have to adjust the trimpot
 *  to even see it).  The logic works fine at 3.3 volts,
 *  but to see the display, it really wants 5 volts.
 */

// Comment this out to use this as a library
// #define RUN_AS_DEMO

#include <unwired.h>
#include <i2c.h>

/* The idea here is that if we don't define this,
 * we can use this as part of a bigger project.
 */
#define RUN_AS_DEMO

/* On 11-14-2020 I switched to using the bit-bang driver
 * and adopted my Kyu-like i2c API in an "official" way.
 */

/* No reason this shouldn't be global */
static struct i2c *ip;

void lcd_init ( struct i2c *ip );
void lcd_begin ( void );
void lcd_msg ( struct i2c *ip, char *msg );
void lcd_msg2 ( struct i2c *ip, char *msg );

void lcd_test_loop ( struct i2c *ip );

void
lcd_begin ( void )
{
#ifdef notdef
	/* Very important */
	i2c_master_enable ( I2C2, 0);
#endif
	printf ( " ---- Booted\n" );

	/* D30 is sda, D29 is sclk */
	ip = i2c_gpio_new ( D30, D29 );
	if ( ! ip ) {
	    printf ( "Cannot set up GPIO iic\n" );
	    spin ();
	}

	// printf ( "Start init sequence\n" );

	lcd_init ( ip );

	// printf ( "Finished init sequence\n" );
}

#ifdef RUN_AS_DEMO
void
lcd_test ( void )
{
	printf ( "Testing LCD\n" );

	lcd_msg ( ip, "Eat more fish" );
	// lcd_msg2 ( ip, "GPS 5244" );
	delay ( 1000 );

	// lcd_loop ( ip );

	printf ( "Enter test loop\n" );

	for ( ;; ) {
	    lcd_msg ( ip, "Eat more fish" );
	    lcd_msg2 ( ip, "GPS 5244" );
	    delay ( 500 );

	    lcd_msg ( ip, "GPS 5244" );
	    lcd_msg2 ( ip, "Eat more fish" );
	    delay ( 500 );
	}

	printf ( "Done testing LCD\n" );
	spin ();
}

void
main(void)
{
	int fd;

	fd = serial_begin ( SERIAL_1, 115200 );
	set_std_serial ( fd );

	printf ( "-- Booted: ready to go\n" );

	lcd_begin ();

	// lcd_test ();
	// lcd_loop ( ip );
	lcd_test_loop ( ip );
}
#endif

#ifdef notdef
/* Also bogus for now.
 * This was an early attempt to use the buggy libmaple/hardware driver.
 * This is a kyu API.
 */
void
i2c_send ( struct i2c *ip, int addr, unsigned char *buf, int count )
{
    i2c_msg write_msg;

    // printf ( "Start i2c_send\n" );

    write_msg.addr = addr;
    write_msg.flags = 0; // write, 7 bit address
    write_msg.length = count;
    write_msg.xferred = 0;
    // write_msg.data = write_buffer;
    write_msg.data = buf;

    // write_buffer[0] = buf[0];

    i2c_master_xfer ( I2C2, &write_msg, 1, 2000);
    // printf ( "Finished i2c_send\n" );
}
#endif

/* It is pretty much unchanged Kyu code below here */

/* ---------------------------------------------- */
/* ---------------------------------------------- */

/* i2c LCD driver
 */
#define LCD_ADDR	0x27

#define LCD_LIGHT	0x08	/* turn backlight LED on */
#define LCD_ENA		0x04	/* for strobe pulse */
#define LCD_READ	0x02	/* we never read */
#define LCD_DATA	0x01	/* 0 is command */

#define LCD_WIDTH	16

/* Base addresses for up to 4 lines (we have only 2)
 */
#define LCD_LINE_1	0x80
#define LCD_LINE_2	0xC0
// #define LCD_LINE_3	0x94
// #define LCD_LINE_4	0xD4

static int use_led = LCD_LIGHT;
// static int use_led = 0;

/* Commands to the HD44780 */

/* The following will get sent to put the device
 * into 4 bit mode.  We don't know what mode it is
 * in when we get hold of it, but sending 0011
 * three times will put it into 8 bit mode.
 * Then sending 0010 will put it into 4 bit mode,
 * which is what we want.
 */
#define INIT1		0x33
#define INIT2		0x32

/* This sets two bits (the low 2), which are "IS"
 * I = 1 says to increment by one in display ram when a char is written.
 * S = 0 says to NOT shift the display on a write.
 */
#define INIT_CURSOR	0x06

/* This sets three bits (the low three), which are "DCB"
 * D = 1 says to turn on the display
 * C = 0 says no cursor.
 * B = 0 says no blink.
 */
#define INIT_DISPLAY	0x0c

/* The two low bits are "don't care", then we have 3 bits "DNF"
 * D = 0  says to use 4 bit mode
 * N = 1  says there are 2 lines (rather than one)
 * F = 0  says use 5x8 font (rather than 5x10)
 */
#define INIT_FUNC	0x28

#define CLEAR_DISPLAY	0x01

#define DELAY_INIT	500
// #define DELAY_NORM	500	ok
// #define DELAY_NORM	400	ok
// #define DELAY_NORM	200	too short
// #define DELAY_NORM	250	too short
#define DELAY_NORM	300

/* ------------------------------------------------------- */

/* The 8574 is pleasantly simple.
 * no complex initialization or setup,
 * just write 8 bits to it and they appears on the output pins.
 */

static void
lcd_write ( struct i2c *ip, int data )
{
	unsigned char buf[2];

	buf[0] = data;
	i2c_send ( ip, LCD_ADDR, buf, 1 );

	/* tjt - needed on stm32 for some reason */
	// delay_us ( 200 ); -- OK
	delay_us ( 50 );
	// delay_us ( 40 ); -- not enough
	// delay_us ( 20 ); -- not enough
	// delay_us ( 10 ); - not enough
}

/* Send data, strobe the ENA line.
 * The shortest possible pulse works fine.
 * There is no need for extra delays.
 */
static void
lcd_send ( struct i2c *ip, int data )
{
	lcd_write ( ip, data );
	lcd_write ( ip, data | LCD_ENA );

	// extra delay
	// delay_us ( 50 );

	lcd_write ( ip, data );

	delay_us ( DELAY_NORM );
}

/* Send data to the display */
static void
lcd_data ( struct i2c *ip, int data )
{
	int hi, lo;

	hi = use_led | LCD_DATA | (data & 0xf0);
	lo = use_led | LCD_DATA | ((data<<4) & 0xf0);

	lcd_send ( ip, hi );
	lcd_send ( ip, lo );
}

/* Send configuration commands to the unit */
static void
lcd_cmd ( struct i2c *ip, int data )
{
	int hi, lo;

	hi = use_led | (data & 0xf0);
	lo = use_led | ((data<<4) & 0xf0);

	lcd_send ( ip, hi );
	lcd_send ( ip, lo );
}

static void
lcd_string ( struct i2c *ip, char *msg )
{
	int i;

	for ( i=0; i<LCD_WIDTH; i++ ) {
	    if ( *msg )
		lcd_data ( ip, *msg++ );
	    else
		lcd_data ( ip, ' ' );
	}
}

void
lcd_msg ( struct i2c *ip, char *msg )
{
	lcd_cmd ( ip, LCD_LINE_1 );
	lcd_string ( ip, msg );
}

void
lcd_msg2 ( struct i2c *ip, char *msg )
{
	lcd_cmd ( ip, LCD_LINE_2 );
	lcd_string ( ip, msg );
}

/* Get it into 4 bit mode */
void
lcd_init_4 ( struct i2c *ip )
{
	/* get into 4 bit mode */
	lcd_cmd ( ip, INIT1 );
	lcd_cmd ( ip, INIT2 );
}

void
lcd_init ( struct i2c *ip )
{
	lcd_init_4 ( ip );

	lcd_cmd ( ip, INIT_CURSOR );
	lcd_cmd ( ip, INIT_DISPLAY );
	lcd_cmd ( ip, INIT_FUNC );

	lcd_cmd ( ip, CLEAR_DISPLAY );

	// important
	delay_us ( DELAY_INIT );
}

/* As above, but don't clear display */
void
lcd_reinit ( struct i2c *ip )
{
	lcd_init_4 ( ip );

	// printf ( "init_4 OK\n" );

	lcd_cmd ( ip, INIT_CURSOR );
	lcd_cmd ( ip, INIT_DISPLAY );
	lcd_cmd ( ip, INIT_FUNC );

	// lcd_cmd ( ip, CLEAR_DISPLAY );

	// important
	delay_us ( DELAY_INIT );
}

/* Originally run for 2,000,000+ cycles on the orange pi
 */
void
lcd_test_loop ( struct i2c *ip )
{
        static int test_state = 0;
        int count;
        char buf[32];

        count = 0;
        for ( ;; ) {
            // thr_delay ( 1000 );
            delay ( 500 );
            count++;
            if ( test_state ) {
                lcd_msg ( ip, "Be nice" );
                test_state = 0;
            } else {
                lcd_msg ( ip, "Pray more often" );
                test_state = 1;
            }
            sprintf ( buf, "  %d", count );
            lcd_msg2 ( ip, buf );
        }
}

void
lcd_loop ( struct i2c *ip )
{
	char msg[17];
	int i;

	for ( i=0; i<LCD_WIDTH; i++ )
	    msg[i] = ' ';
	msg[LCD_WIDTH] = '\0';

	msg[0] = '*';
	lcd_msg2 ( ip, msg );
	// thr_delay ( 200 );
	delay ( 200 );

	for ( ;; ) {
	    for ( i=1; i<LCD_WIDTH; i++ ) {
		if ( i > 0 )
		    msg[i-1] = ' ';
		msg[i] = '*';
		lcd_msg2 ( ip, msg );
		// thr_delay ( 200 );
		delay ( 200 );
	    }
	    for ( i=LCD_WIDTH-2; i >= 0; i-- ) {
		msg[i+1] = ' ';
		msg[i] = '*';
		lcd_msg2 ( ip, msg );
		// thr_delay ( 200 );
		delay ( 200 );
	    }
	}
}

/* THE END */

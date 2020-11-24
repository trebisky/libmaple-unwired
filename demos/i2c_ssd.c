/******************************************************************************
 SSD1306 OLED driver for CCS PIC C compiler (SSD1306OLED.c)                   *
 Reference: Adafruit Industries SSD1306 OLED driver and graphics library.     *
                                                                              *
 The driver is for I2C mode only.                                             *
                                                                              *
 https://simple-circuit.com/                                                   *
                                                                              *
*******************************************************************************
*******************************************************************************
 This is a library for our Monochrome OLEDs based on SSD1306 drivers          *
                                                                              *
  Pick one up today in the adafruit shop!                                     *
  ------> http://www.adafruit.com/category/63_98                              *
                                                                              *
 Adafruit invests time and resources providing this open source code,         *
 please support Adafruit and open-source hardware by purchasing               *
 products from Adafruit!                                                      *
                                                                              *
 Written by Limor Fried/Ladyada  for Adafruit Industries.                     *
 BSD license, check license.txt for more information                          *
 All text above, and the splash screen must be included in any redistribution *
*******************************************************************************/

#define FALSE	0
#define TRUE	1

// #include <stdint.h>

#include <unwired.h>
#include <i2c.h>

static struct i2c *ip;

/* Prototypes */
void ssd_init ( struct i2c * );
void ssd_set_pixel ( int, int );
void ssd_clear_pixel ( int, int );
void ssd_clear_all ( void );
void ssd_display ( void );
void ssd_putc ( int );
void ssd_text ( int, int, char * );
void ssd_set_textsize ( int );

static void ssd1306_command ( int );
static void SSD1306_Begin ( void );

// =================================================================

// Doesn't belong here.
// experiments with how the compiler generates code.

static int index = 0;
static int vals[] = { 12, 20, 33 };

int dingle ( void )
{
    index = (index+1) % 3;
    return vals[index];
}

int8_t dongle ( void )
{
    index = (index+2) % 3;
    return vals[index];
}

static volatile int8_t sizzle;

void
size_test ( void )
{
    sizzle = dingle ();
    sizzle = dongle ();
}

// =================================================================

/* My devices are 128 by 64 */
#define SSD1306_128_64

#define WIDTH            128
#define HEIGHT            64
// #define LINE_SIZE	(WIDTH/8)
#define LINE_SIZE	16
#define BUF_SIZE	(HEIGHT * LINE_SIZE)

//------------------------------ Definitions ---------------------------------//

/* This is the real i2c address, the other values
 * are shifted left 1 bit, causing no end of confusion.
 */
#define SSD1306_I2C_ADDRESS   0x3C

// #define SSD1306_I2C_ADDRESS   0x78
//#define SSD1306_I2C_ADDRESS   0x7A

#ifdef OLD_GEOMETRY
/* -- */
#if !defined SSD1306_128_32 && !defined SSD1306_96_16
#define SSD1306_128_64
#endif
#if defined SSD1306_128_32 && defined SSD1306_96_16
  #error "Only one SSD1306 display can be specified at once"
#endif

#if defined SSD1306_128_64
  #define SSD1306_LCDWIDTH            128
  #define SSD1306_LCDHEIGHT            64
  #define LINE_SIZE	16
#endif
#if defined SSD1306_128_32
  #define SSD1306_LCDWIDTH            128
  #define SSD1306_LCDHEIGHT            32
  #define LINE_SIZE	16
#endif
#if defined SSD1306_96_16
  #define SSD1306_LCDWIDTH             96
  #define SSD1306_LCDHEIGHT            16
  #define LINE_SIZE	12
#endif
#endif /* OLD_GEOMETRY */

#define SSD1306_SETCONTRAST          0x81
#define SSD1306_DISPLAYALLON_RESUME  0xA4
#define SSD1306_DISPLAYALLON         0xA5
#define SSD1306_NORMALDISPLAY        0xA6
#define SSD1306_INVERTDISPLAY_       0xA7
#define SSD1306_DISPLAYOFF           0xAE
#define SSD1306_DISPLAYON            0xAF
#define SSD1306_SETDISPLAYOFFSET     0xD3
#define SSD1306_SETCOMPINS           0xDA
#define SSD1306_SETVCOMDETECT        0xDB
#define SSD1306_SETDISPLAYCLOCKDIV   0xD5
#define SSD1306_SETPRECHARGE         0xD9
#define SSD1306_SETMULTIPLEX         0xA8
#define SSD1306_SETLOWCOLUMN         0x00
#define SSD1306_SETHIGHCOLUMN        0x10
#define SSD1306_SETSTARTLINE         0x40
#define SSD1306_MEMORYMODE           0x20
#define SSD1306_COLUMNADDR           0x21
#define SSD1306_PAGEADDR             0x22
#define SSD1306_COMSCANINC           0xC0
#define SSD1306_COMSCANDEC           0xC8
#define SSD1306_SEGREMAP             0xA0
#define SSD1306_CHARGEPUMP           0x8D
#define SSD1306_EXTERNALVCC          0x01
#define SSD1306_SWITCHCAPVCC         0x02

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL                      0x2F
#define SSD1306_DEACTIVATE_SCROLL                    0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA             0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A

#define ssd1306_swap(a, b) { int t = a; a = b; b = t; }

#define abs(x) ({                                               \
                long ret;                                       \
                if (sizeof(x) == sizeof(long)) {                \
                        long __x = (x);                         \
                        ret = (__x < 0) ? -__x : __x;           \
                } else {                                        \
                        int __x = (x);                          \
                        ret = (__x < 0) ? -__x : __x;           \
                }                                               \
                ret;                                            \
        })


// uint8_t _i2caddr;
uint8_t _vccstate;
uint8_t  x_pos, y_pos;
uint8_t  text_size;
int8_t   wrap = TRUE;


//--------------------------------------------------------------------------//

const char Font[] = {
0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x5F, 0x00, 0x00,
0x00, 0x07, 0x00, 0x07, 0x00,
0x14, 0x7F, 0x14, 0x7F, 0x14,
0x24, 0x2A, 0x7F, 0x2A, 0x12,
0x23, 0x13, 0x08, 0x64, 0x62,
0x36, 0x49, 0x56, 0x20, 0x50,
0x00, 0x08, 0x07, 0x03, 0x00,
0x00, 0x1C, 0x22, 0x41, 0x00,
0x00, 0x41, 0x22, 0x1C, 0x00,
0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
0x08, 0x08, 0x3E, 0x08, 0x08,
0x00, 0x80, 0x70, 0x30, 0x00,
0x08, 0x08, 0x08, 0x08, 0x08,
0x00, 0x00, 0x60, 0x60, 0x00,
0x20, 0x10, 0x08, 0x04, 0x02,
0x3E, 0x51, 0x49, 0x45, 0x3E,
0x00, 0x42, 0x7F, 0x40, 0x00,
0x72, 0x49, 0x49, 0x49, 0x46,
0x21, 0x41, 0x49, 0x4D, 0x33,
0x18, 0x14, 0x12, 0x7F, 0x10,
0x27, 0x45, 0x45, 0x45, 0x39,
0x3C, 0x4A, 0x49, 0x49, 0x31,
0x41, 0x21, 0x11, 0x09, 0x07,
0x36, 0x49, 0x49, 0x49, 0x36,
0x46, 0x49, 0x49, 0x29, 0x1E,
0x00, 0x00, 0x14, 0x00, 0x00,
0x00, 0x40, 0x34, 0x00, 0x00,
0x00, 0x08, 0x14, 0x22, 0x41,
0x14, 0x14, 0x14, 0x14, 0x14,
0x00, 0x41, 0x22, 0x14, 0x08,
0x02, 0x01, 0x59, 0x09, 0x06,
0x3E, 0x41, 0x5D, 0x59, 0x4E,
0x7C, 0x12, 0x11, 0x12, 0x7C,
0x7F, 0x49, 0x49, 0x49, 0x36,
0x3E, 0x41, 0x41, 0x41, 0x22,
0x7F, 0x41, 0x41, 0x41, 0x3E,
0x7F, 0x49, 0x49, 0x49, 0x41,
0x7F, 0x09, 0x09, 0x09, 0x01,
0x3E, 0x41, 0x41, 0x51, 0x73,
0x7F, 0x08, 0x08, 0x08, 0x7F,
0x00, 0x41, 0x7F, 0x41, 0x00,
0x20, 0x40, 0x41, 0x3F, 0x01,
0x7F, 0x08, 0x14, 0x22, 0x41,
0x7F, 0x40, 0x40, 0x40, 0x40,
0x7F, 0x02, 0x1C, 0x02, 0x7F,
0x7F, 0x04, 0x08, 0x10, 0x7F,
0x3E, 0x41, 0x41, 0x41, 0x3E,
0x7F, 0x09, 0x09, 0x09, 0x06,
0x3E, 0x41, 0x51, 0x21, 0x5E,
0x7F, 0x09, 0x19, 0x29, 0x46
};

const char Font2[] = {
0x26, 0x49, 0x49, 0x49, 0x32,
0x03, 0x01, 0x7F, 0x01, 0x03,
0x3F, 0x40, 0x40, 0x40, 0x3F,
0x1F, 0x20, 0x40, 0x20, 0x1F,
0x3F, 0x40, 0x38, 0x40, 0x3F,
0x63, 0x14, 0x08, 0x14, 0x63,
0x03, 0x04, 0x78, 0x04, 0x03,
0x61, 0x59, 0x49, 0x4D, 0x43,
0x00, 0x7F, 0x41, 0x41, 0x41,
0x02, 0x04, 0x08, 0x10, 0x20,
0x00, 0x41, 0x41, 0x41, 0x7F,
0x04, 0x02, 0x01, 0x02, 0x04,
0x40, 0x40, 0x40, 0x40, 0x40,
0x00, 0x03, 0x07, 0x08, 0x00,
0x20, 0x54, 0x54, 0x78, 0x40,
0x7F, 0x28, 0x44, 0x44, 0x38,
0x38, 0x44, 0x44, 0x44, 0x28,
0x38, 0x44, 0x44, 0x28, 0x7F,
0x38, 0x54, 0x54, 0x54, 0x18,
0x00, 0x08, 0x7E, 0x09, 0x02,
0x18, 0xA4, 0xA4, 0x9C, 0x78,
0x7F, 0x08, 0x04, 0x04, 0x78,
0x00, 0x44, 0x7D, 0x40, 0x00,
0x20, 0x40, 0x40, 0x3D, 0x00,
0x7F, 0x10, 0x28, 0x44, 0x00,
0x00, 0x41, 0x7F, 0x40, 0x00,
0x7C, 0x04, 0x78, 0x04, 0x78,
0x7C, 0x08, 0x04, 0x04, 0x78,
0x38, 0x44, 0x44, 0x44, 0x38,
0xFC, 0x18, 0x24, 0x24, 0x18,
0x18, 0x24, 0x24, 0x18, 0xFC,
0x7C, 0x08, 0x04, 0x04, 0x08,
0x48, 0x54, 0x54, 0x54, 0x24,
0x04, 0x04, 0x3F, 0x44, 0x24,
0x3C, 0x40, 0x40, 0x20, 0x7C,
0x1C, 0x20, 0x40, 0x20, 0x1C,
0x3C, 0x40, 0x30, 0x40, 0x3C,
0x44, 0x28, 0x10, 0x28, 0x44,
0x4C, 0x90, 0x90, 0x90, 0x7C,
0x44, 0x64, 0x54, 0x4C, 0x44,
0x00, 0x08, 0x36, 0x41, 0x00,
0x00, 0x00, 0x77, 0x00, 0x00,
0x00, 0x41, 0x36, 0x08, 0x00,
0x02, 0x01, 0x02, 0x04, 0x02
};

#ifndef ADAFRUIT_LOGO
static uint8_t ssd1306_buffer[BUF_SIZE];
#else
/* This loads the buffer with the Adafruit logo */
static uint8_t ssd1306_buffer[BUF_SIZE] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x80, 0x80, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xF8, 0xE0, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80,
0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0xFF
#if (SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH > 96*16)
,
0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00,
0x80, 0xFF, 0xFF, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x8C, 0x8E, 0x84, 0x00, 0x00, 0x80, 0xF8,
0xF8, 0xF8, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xE0, 0xE0, 0xC0, 0x80,
0x00, 0xE0, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xC7, 0x01, 0x01,
0x01, 0x01, 0x83, 0xFF, 0xFF, 0x00, 0x00, 0x7C, 0xFE, 0xC7, 0x01, 0x01, 0x01, 0x01, 0x83, 0xFF,
0xFF, 0xFF, 0x00, 0x38, 0xFE, 0xC7, 0x83, 0x01, 0x01, 0x01, 0x83, 0xC7, 0xFF, 0xFF, 0x00, 0x00,
0x01, 0xFF, 0xFF, 0x01, 0x01, 0x00, 0xFF, 0xFF, 0x07, 0x01, 0x01, 0x01, 0x00, 0x00, 0x7F, 0xFF,
0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0xFF,
0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x03, 0x0F, 0x3F, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xC7, 0xC7, 0x8F,
0x8F, 0x9F, 0xBF, 0xFF, 0xFF, 0xC3, 0xC0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFC, 0xFC,
0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8, 0xF0, 0xF0, 0xE0, 0xC0, 0x00, 0x01, 0x03, 0x03, 0x03,
0x03, 0x03, 0x01, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01,
0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x03, 0x03, 0x00, 0x00,
0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x03, 0x01, 0x00, 0x00, 0x00, 0x03,
0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#if (SSD1306_LCDHEIGHT == 64)
,
0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x1F, 0x0F,
0x87, 0xC7, 0xF7, 0xFF, 0xFF, 0x1F, 0x1F, 0x3D, 0xFC, 0xF8, 0xF8, 0xF8, 0xF8, 0x7C, 0x7D, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x3F, 0x0F, 0x07, 0x00, 0x30, 0x30, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xC0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xC0, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x3F, 0x1F,
0x0F, 0x07, 0x1F, 0x7F, 0xFF, 0xFF, 0xF8, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xE0,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0x00, 0x00,
0x00, 0xFC, 0xFE, 0xFC, 0x0C, 0x06, 0x06, 0x0E, 0xFC, 0xF8, 0x00, 0x00, 0xF0, 0xF8, 0x1C, 0x0E,
0x06, 0x06, 0x06, 0x0C, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFE, 0xFE, 0x00, 0x00, 0x00, 0x00, 0xFC,
0xFE, 0xFC, 0x00, 0x18, 0x3C, 0x7E, 0x66, 0xE6, 0xCE, 0x84, 0x00, 0x00, 0x06, 0xFF, 0xFF, 0x06,
0x06, 0xFC, 0xFE, 0xFC, 0x0C, 0x06, 0x06, 0x06, 0x00, 0x00, 0xFE, 0xFE, 0x00, 0x00, 0xC0, 0xF8,
0xFC, 0x4E, 0x46, 0x46, 0x46, 0x4E, 0x7C, 0x78, 0x40, 0x18, 0x3C, 0x76, 0xE6, 0xCE, 0xCC, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x0F, 0x1F, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x0F, 0x03,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00,
0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x03, 0x07, 0x0E, 0x0C,
0x18, 0x18, 0x0C, 0x06, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x01, 0x0F, 0x0E, 0x0C, 0x18, 0x0C, 0x0F,
0x07, 0x01, 0x00, 0x04, 0x0E, 0x0C, 0x18, 0x0C, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00,
0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x07,
0x07, 0x0C, 0x0C, 0x18, 0x1C, 0x0C, 0x06, 0x06, 0x00, 0x04, 0x0E, 0x0C, 0x18, 0x0C, 0x0F, 0x07,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#endif
#endif
};
#endif

#ifdef RUN_AS_DEMO


void gb_init_rand ( int );
int gb_unif_rand ( int );

static void
random_bits ( int num )
{
	int x,y;
	int i;

	ssd_clear_all ();
	for ( i=0; i<num; i++ ) {
	    x = gb_unif_rand(WIDTH);
	    y = gb_unif_rand(HEIGHT);
	    ssd_set_pixel ( x, y );
	}
	ssd_display ();
}

static void
test1 ( void )
{
	gb_init_rand ( 0x163389 );

	for ( ;; ) {
	    random_bits ( 50 );
	    delay ( 200 );
	}

}

static void
test2 ( void )
{
	int i;
	char buf[10];
	int elev;

	ssd_clear_all ();

	ssd_set_textsize ( 4 );

	for ( elev=9999; ; elev++ ) {
	    sprintf ( buf, "%d", elev );
	    ssd_text ( 0, 18, buf );
	    ssd_display ();
	    delay ( 1000 );
	}

	// for ( i=0; i < 8; i++ ) {
	    // ssd_text ( 0, i, "Hello World" );
	// }

}

/* Two numbers, one above the other */
static void
test3 ( void )
{
	int i;
	char buf[10];
	int elev;

	ssd_clear_all ();

	ssd_set_textsize ( 4 );

	for ( elev=9999; ; elev++ ) {
	    sprintf ( buf, "%d", elev );
	    ssd_text ( 0, 0, buf );
	    ssd_text ( 0, 36, buf );
	    ssd_display ();
	    delay ( 1000 );
	}
}

/* Two numbers, big above small */
static void
test4 ( void )
{
	int i;
	char buf[10];
	char *time = "12:53:17";
	int elev;

	ssd_clear_all ();


	for ( elev=9999; ; elev++ ) {
	    sprintf ( buf, "%d", elev );
	    ssd_set_textsize ( 4 );
	    ssd_text ( 0, 0, buf );
	    ssd_set_textsize ( 2 );
	    ssd_text ( 10, 49, time );
	    ssd_display ();
	    delay ( 1000 );
	}
}


void
main ( void )
{
	struct i2c *xip;

	// size_test ();

	/* D30 is sda, D29 is sclk */
        // ip = i2c_gpio_new ( D30, D29 );

	/* For blue pill */
        xip = i2c_gpio_new ( PB11, PB10 );

        if ( ! xip ) {
            printf ( "Cannot set up GPIO iic\n" );
            spin ();
        }

	printf ( "Initalize SSD\n" );
	ssd_init ( xip );

#ifdef notdef
	printf ( "Load display\n" );
	ssd_display ();
	/* Time for "flash screen * */
	delay ( 500 );
#endif
	printf ( "Run SSD test\n" );

	// test1 ();
	// test2 ();
	// test3 ();
	test4 ();

	spin ();
}
#endif /* RUN_AS_DEMO */

void
ssd_init ( struct i2c *aip )
{
    ip = aip;

    SSD1306_Begin ();
}

void 
ssd1306_command ( int c )
{
    unsigned char io_buf[2];

    // uint8_t control = 0x00;   // Co = 0, D/C = 0
    // I2C_Start(SSD1306_STREAM);

    // I2C_Write(SSD1306_STREAM, _i2caddr);
    // I2C_Write(SSD1306_STREAM, control);
    // I2C_Write(SSD1306_STREAM, c);

    io_buf[0] = 0x00;
    io_buf[1] = c;
    i2c_send ( ip, SSD1306_I2C_ADDRESS, io_buf, 2 );

    // I2C_Stop(SSD1306_STREAM);
}

/* We always send to the display 16 bytes at a time.
 * 16 * 8 = 128 bits
 */
static void
ssd_sendbuf ( unsigned char *buf )
{
    unsigned char io_buf[LINE_SIZE+1];
    int i;

    io_buf[0] = 0x40;
    for ( i=0; i < LINE_SIZE; i++ )
	io_buf[i+1] = buf[i];
    i2c_send ( ip, SSD1306_I2C_ADDRESS, io_buf, LINE_SIZE+1 );
}

// void SSD1306_Display(void)
void
ssd_display ( void )
{
  int i;

  ssd1306_command(SSD1306_COLUMNADDR);
  ssd1306_command(0);			// Column start address (0 = reset)
  ssd1306_command(WIDTH-1);	// Column end address (127 = reset)

  ssd1306_command(SSD1306_PAGEADDR);
  ssd1306_command(0);			// Page start address (0 = reset)

  #if HEIGHT == 64
    ssd1306_command(7);			// Page end address
  #endif
  #if HEIGHT == 32
    ssd1306_command(3);			// Page end address
  #endif
  #if HEIGHT == 16
    ssd1306_command(1);			// Page end address
  #endif

  /* On my 128x64 unit, this loop grinds out 64 lines one by one */
  for ( i = 0; i < BUF_SIZE; i += LINE_SIZE )
      ssd_sendbuf ( &ssd1306_buffer[i] );
}

static void
SSD1306_Begin ( void )
{
  _vccstate = SSD1306_SWITCHCAPVCC;
  // _i2caddr  = SSD1306_I2C_ADDRESS;

  // Why ?
  // There actually could be a reason for this.
  // I have seen my display not be ready to go when first
  // power is applied (but never just after a reboot/reset).
  // The SSD1306 does its own reset, and there is a race.
  // Usually the processor gets here first,
  // so the delay is required.
  // delay(10);
  delay(500);

  // Init sequence
  ssd1306_command(SSD1306_DISPLAYOFF);                    // 0xAE
  ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
  ssd1306_command(0x80);                                  // the suggested ratio 0x80

  ssd1306_command(SSD1306_SETMULTIPLEX);                  // 0xA8
  ssd1306_command(HEIGHT - 1);

  ssd1306_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
  ssd1306_command(0x0);                                   // no offset
  ssd1306_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
  ssd1306_command(SSD1306_CHARGEPUMP);                    // 0x8D

  if (_vccstate == SSD1306_EXTERNALVCC)
    { ssd1306_command(0x10); }
  else
    { ssd1306_command(0x14); }

  ssd1306_command(SSD1306_MEMORYMODE);                    // 0x20
  ssd1306_command(0x00);                                  // 0x0 act like ks0108
  ssd1306_command(SSD1306_SEGREMAP | 0x1);
  ssd1306_command(SSD1306_COMSCANDEC);

#if defined SSD1306_128_32
  ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
  ssd1306_command(0x02);
  ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
  ssd1306_command(0x8F);

/* tjt -- my units !! */
#elif defined SSD1306_128_64
  ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
  ssd1306_command(0x12);
  ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
  if (_vccstate == SSD1306_EXTERNALVCC)
    { ssd1306_command(0x9F); }
  else
    { ssd1306_command(0xCF); }

#elif defined SSD1306_96_16
  ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
  ssd1306_command(0x2);   //ada x12
  ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
  if (_vccstate == SSD1306_EXTERNALVCC)
    { ssd1306_command(0x10); }
  else
    { ssd1306_command(0xAF); }

#endif

  ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9

  if (_vccstate == SSD1306_EXTERNALVCC)
    { ssd1306_command(0x22); }
  else
    { ssd1306_command(0xF1); }

  ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
  ssd1306_command(0x40);
  ssd1306_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
  ssd1306_command(SSD1306_NORMALDISPLAY);                 // 0xA6

  ssd1306_command(SSD1306_DEACTIVATE_SCROLL);

  ssd1306_command(SSD1306_DISPLAYON);//--turn on oled panel
  
  // set cursor to (0, 0)
  x_pos = 0;
  y_pos = 0;

  // set text size to 1
  text_size = 1;
}

// void SSD1306_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color = TRUE)
void
ssd_draw_line ( int x0, int y0, int x1, int y1, int color )
{
  int steep;
  int ystep;
  int dx, dy;
  int err;

  steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    ssd1306_swap(x0, y0);
    ssd1306_swap(x1, y1);
  }
  if (x0 > x1) {
    ssd1306_swap(x0, x1);
    ssd1306_swap(y0, y1);
  }
  dx = x1 - x0;
  dy = abs(y1 - y0);

  err = dx / 2;
  if (y0 < y1)
    ystep = 1;
  else
    ystep = -1;

  for (; x0 <= x1; x0++) {
    if (steep) {
      if(color)
	  ssd_set_pixel ( y0, x0 );
      else
	  ssd_clear_pixel ( y0, x0 );
    } else {
      if(color)
	  ssd_set_pixel ( x0, y0 );
      else
	  ssd_clear_pixel ( x0, y0 );
    }
    err -= dy;
    if (err < 0) {
      y0  += ystep;
      err += dx;
    }
  }
}


// void SSD1306_DrawFastHLine(uint8_t x, uint8_t y, uint8_t w, int8_t color = TRUE)
void
ssd_hline ( int x, int y, int w, int color )
{
   ssd_draw_line (x, y, x + w - 1, y, color);
}

// void SSD1306_DrawFastVLine(uint8_t x, uint8_t y, uint8_t h, int8_t color = TRUE)
void
ssd_vline ( int x, int y, int h, int color )
{
  ssd_draw_line (x, y, x, y + h - 1, color);
}

// void SSD1306_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, int8_t color = TRUE)
void
ssd_fill_rect ( int x, int y, int w, int h, int color )
{
  int i;

  for ( i = x; i < x + w; i++)
    ssd_vline (i, y, h, color);
}

// Hard to see why you would use this rather than clear_all.
// void SSD1306_FillScreen(int8_t color = TRUE) {
void
ssd_fill_screen ( int color)
{
  ssd_fill_rect (0, 0, WIDTH, HEIGHT, color);
}

void
ssd_clear_all ( void )
{
  int i;

  for (i = 0; i < BUF_SIZE; i++)
    ssd1306_buffer[i] = 0;
}

void
ssd_set_pixel ( int x, int y )
{
  if ((x >= WIDTH) || (y >= HEIGHT))
    return;
  ssd1306_buffer[x + (int)(y / 8) * WIDTH] |=  (1 << (y & 7));
}

void
ssd_clear_pixel ( int x, int y )
{
  if ((x >= WIDTH) || (y >= HEIGHT))
    return;
  ssd1306_buffer[x + (int)(y / 8) * WIDTH] &=  ~(1 << (y & 7));
}

void
ssd_draw_pixel ( int x, int y, int color )
{
    if ( color )
	ssd_set_pixel ( x, y );
    else
	ssd_clear_pixel ( x, y );
}

// void SSD1306_SetTextWrap(int8_t w)
void
ssd_set_wrap ( int w )
{
  wrap = w;
}

// void SSD1306_TextSize(uint8_t t_size)
void
ssd_set_textsize ( int size )
{
  if ( size < 1 ) size = 1;
  text_size = size;
}

// move cursor to position (x, y)
// void SSD1306_GotoXY(uint8_t x, uint8_t y)
void
ssd_xy ( int x, int y )
{
  if ( (x >= WIDTH) || (y >= HEIGHT) )
    return;
  x_pos = x;
  y_pos = y;
}

// void SSD1306_DrawChar(uint8_t x, uint8_t y, uint8_t c, uint8_t size = 1)
void
ssd_drawchar ( int x, int y, int c )
{
  ssd_xy ( x, y );
  // SSD1306_TextSize(size);
  ssd_putc ( c );
}

// void SSD1306_DrawText(uint8_t x, uint8_t y, char *_text, uint8_t size = 1)
void
ssd_text ( int x, int y, char *msg )
{
  ssd_xy ( x, y );
  // SSD1306_TextSize(size);
  while ( *msg )
    ssd_putc ( *msg++ );

}

/* print single char
    \a  Set cursor position to upper left (0, 0)
    \b  Move back one position
    \n  Go to start of current line
    \r  Go to line below
*/
// void SSD1306_Print(uint8_t c)
void
ssd_putc ( int c )
{
  int _color;
  int i, j, line;
  
  if (c == ' ' && x_pos == 0 && wrap)
    return;

  if(c == '\a') {
    x_pos = y_pos = 0;
    return;
  }

  if( (c == '\b') && (x_pos >= text_size * 6) ) {
    x_pos -= text_size * 6;
    return;
  }

  if(c == '\r') {
    x_pos = 0;
    return;
  }

  if(c == '\n') {
    y_pos += text_size * 8;
    if((y_pos + text_size * 7) > HEIGHT)
      y_pos = 0;
    return;
  }

  if((c < ' ') || (c > '~'))
    c = '?';
  
  for(i = 0; i < 5; i++ ) {
    if(c < 'S')
      line = Font[(c - ' ') * 5 + i];
    else
      line = Font2[(c - 'S') * 5 + i];
    
    for(j = 0; j < 7; j++, line >>= 1) {
      if(line & 0x01)
        _color = TRUE;
      else
        _color = FALSE;

      if(text_size == 1)
	  ssd_draw_pixel ( x_pos + i, y_pos + j, _color );
      else
	  ssd_fill_rect (x_pos + (i * text_size), y_pos + (j * text_size), text_size, text_size, _color);
    }
  }

  ssd_fill_rect (x_pos + (5 * text_size), y_pos, text_size, 7 * text_size, FALSE);
  
  x_pos += text_size * 6;

  if( x_pos > (WIDTH + text_size * 6) )
    x_pos = WIDTH;

  if (wrap && (x_pos + (text_size * 5)) > WIDTH)
  {
    x_pos = 0;
    y_pos += text_size * 8;
    if((y_pos + text_size * 7) > HEIGHT)
      y_pos = 0;
  }
}


/* ======================================================================= */
/* ======================================================================= */
/* ======================================================================= */
/* ======================================================================= */

#ifdef WHOLE_BANANA
// tjt
// void SSD1306_Begin(uint8_t vccstate = SSD1306_SWITCHCAPVCC, uint8_t i2caddr = SSD1306_I2C_ADDRESS);
void SSD1306_Begin(uint8_t vccstate = SSD1306_SWITCHCAPVCC, uint8_t i2caddr);
// tjt
// void SSD1306_DrawPixel(uint8_t x, uint8_t y, int8_t color = TRUE);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, int8_t color);
void SSD1306_StartScrollRight(uint8_t start, uint8_t stop);
void SSD1306_StartScrollLeft(uint8_t start, uint8_t stop);
void SSD1306_StartScrollDiagRight(uint8_t start, uint8_t stop);
void SSD1306_StartScrollDiagLeft(uint8_t start, uint8_t stop);
void SSD1306_StopScroll(void);
void SSD1306_Dim(int8_t dim);
void SSD1306_Display(void);
void SSD1306_ClearDisplay(void);
// tjt
// void SSD1306_DrawLine(int8_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color = TRUE);
void SSD1306_DrawLine(int8_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color);
// tjt
// void SSD1306_DrawFastHLine(uint8_t x, uint8_t y, uint8_t w, int8_t color = TRUE);
void SSD1306_DrawFastHLine(uint8_t x, uint8_t y, uint8_t w, int8_t color);
// tjt
// void SSD1306_DrawFastVLine(uint8_t x, uint8_t y, uint8_t h, int8_t color = TRUE);
void SSD1306_DrawFastVLine(uint8_t x, uint8_t y, uint8_t h, int8_t color);
// tjt
// void SSD1306_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, int8_t color = TRUE);
void SSD1306_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, int8_t color);
// tjt
// void SSD1306_FillScreen(int8_t color = TRUE);
void SSD1306_FillScreen(int8_t color);
void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r);
void SSD1306_DrawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername);
// tjt
// void SSD1306_FillCircle(int16_t x0, int16_t y0, int16_t r, int8_t color = TRUE);
void SSD1306_FillCircle(int16_t x0, int16_t y0, int16_t r, int8_t color);
// tjt
// void SSD1306_FillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, int8_t color = TRUE);
void SSD1306_FillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, int8_t color);
void SSD1306_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void SSD1306_DrawRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r);
// tjt
// void SSD1306_FillRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, int8_t color = TRUE);
void SSD1306_FillRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, int8_t color);
void SSD1306_DrawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
// tjt
// void SSD1306_FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int8_t color = TRUE);
void SSD1306_FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int8_t color);
// tjt
// void SSD1306_DrawChar(uint8_t x, uint8_t y, uint8_t c, uint8_t size = 1);
void SSD1306_DrawChar(uint8_t x, uint8_t y, uint8_t c, uint8_t size);
// tjt
// void SSD1306_DrawText(uint8_t x, uint8_t y, char *_text, uint8_t size = 1);
void SSD1306_DrawText(uint8_t x, uint8_t y, char *_text, uint8_t size);

void SSD1306_TextSize(uint8_t t_size);
void SSD1306_GotoXY(uint8_t x, uint8_t y);
void SSD1306_Print(uint8_t c);
void SSD1306_PutCustomC(rom uint8_t *c);
void SSD1306_SetTextWrap(int8_t w);
void SSD1306_InvertDisplay(int8_t i);
void SSD1306_DrawBMP(uint8_t x, uint8_t y, rom uint8_t *bitmap, uint8_t w, uint8_t h);
#endif  /* WHOLE_BANANA */


#ifdef WHOLE_BANANA
void SSD1306_DrawPixel(uint8_t x, uint8_t y, int8_t color = TRUE)
{
  if ((x >= SSD1306_LCDWIDTH) || (y >= SSD1306_LCDHEIGHT))
    return;

  if (color)
    ssd1306_buffer[x + (uint16_t)(y / 8) * SSD1306_LCDWIDTH] |=  (1 << (y & 7));
  else
    ssd1306_buffer[x + (uint16_t)(y / 8) * SSD1306_LCDWIDTH] &=  ~(1 << (y & 7));
}

void SSD1306_StartScrollRight(uint8_t start, uint8_t stop)
{
  ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X00);
  ssd1306_command(0XFF);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

void SSD1306_StartScrollLeft(uint8_t start, uint8_t stop)
{
  ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X00);
  ssd1306_command(0XFF);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

void SSD1306_StartScrollDiagRight(uint8_t start, uint8_t stop)
{
  ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
  ssd1306_command(0X00);
  ssd1306_command(SSD1306_LCDHEIGHT);
  ssd1306_command(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X01);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

void SSD1306_StartScrollDiagLeft(uint8_t start, uint8_t stop)
{
  ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
  ssd1306_command(0X00);
  ssd1306_command(SSD1306_LCDHEIGHT);
  ssd1306_command(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X01);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

void SSD1306_StopScroll(void)
{
  ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
}

void SSD1306_Dim(int8_t dim)
{
  uint8_t contrast;
  if (dim)
    contrast = 0; // Dimmed display
  else {
    if (_vccstate == SSD1306_EXTERNALVCC)
      contrast = 0x9F;
    else
      contrast = 0xCF;
  }
  // the range of contrast to too small to be really useful
  // it is useful to dim the display
  ssd1306_command(SSD1306_SETCONTRAST);
  ssd1306_command(contrast);
}

void SSD1306_Display(void)
{
  ssd1306_command(SSD1306_COLUMNADDR);
  ssd1306_command(0);   // Column start address (0 = reset)
  ssd1306_command(SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

  ssd1306_command(SSD1306_PAGEADDR);
  ssd1306_command(0); // Page start address (0 = reset)
  #if SSD1306_LCDHEIGHT == 64
    ssd1306_command(7); // Page end address
  #endif
  #if SSD1306_LCDHEIGHT == 32
    ssd1306_command(3); // Page end address
  #endif
  #if SSD1306_LCDHEIGHT == 16
    ssd1306_command(1); // Page end address
  #endif
  
  for (uint16_t i = 0; i < (SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT / 8); i++) {
      // send a bunch of data in one xmission
      I2C_Start(SSD1306_STREAM);
      I2C_Write(SSD1306_STREAM, _i2caddr);
      I2C_Write(SSD1306_STREAM, 0x40);
      for (uint8_t x = 0; x < 16; x++) {
        I2C_Write(SSD1306_STREAM, ssd1306_buffer[i]);
        i++;
      }
      i--;
      I2C_Stop(SSD1306_STREAM);
    }
}

void SSD1306_ClearDisplay(void)
{
  for (uint16_t i = 0; i < (SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT / 8); i++)
    ssd1306_buffer[i] = 0;
}

void SSD1306_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color = TRUE)
{
  int8_t steep;
  int8_t ystep;
  uint8_t dx, dy;
  int16_t err;
  steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    ssd1306_swap(x0, y0);
    ssd1306_swap(x1, y1);
  }
  if (x0 > x1) {
    ssd1306_swap(x0, x1);
    ssd1306_swap(y0, y1);
  }
  dx = x1 - x0;
  dy = abs(y1 - y0);

  err = dx / 2;
  if (y0 < y1)
    ystep = 1;
  else
    ystep = -1;

  for (; x0 <= x1; x0++) {
    if (steep) {
      if(color) SSD1306_DrawPixel(y0, x0);
      else      SSD1306_DrawPixel(y0, x0, FALSE);
    }
    else {
      if(color) SSD1306_DrawPixel(x0, y0);
      else      SSD1306_DrawPixel(x0, y0, FALSE);
    }
    err -= dy;
    if (err < 0) {
      y0  += ystep;
      err += dx;
    }
  }
}

void SSD1306_DrawFastHLine(uint8_t x, uint8_t y, uint8_t w, int8_t color = TRUE)
{
   SSD1306_DrawLine(x, y, x + w - 1, y, color);
}

void SSD1306_DrawFastVLine(uint8_t x, uint8_t y, uint8_t h, int8_t color = TRUE)
{
  SSD1306_DrawLine(x, y, x, y + h - 1, color);
}

void SSD1306_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, int8_t color = TRUE)
{
  for (int16_t i = x; i < x + w; i++)
    SSD1306_DrawFastVLine(i, y, h, color);
}

void SSD1306_FillScreen(int8_t color = TRUE) {
  SSD1306_FillRect(0, 0, SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT, color);
}

void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  SSD1306_DrawPixel(x0  , y0 + r);
  SSD1306_DrawPixel(x0  , y0 - r);
  SSD1306_DrawPixel(x0 + r, y0);
  SSD1306_DrawPixel(x0 - r, y0);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    SSD1306_DrawPixel(x0 + x, y0 + y);
    SSD1306_DrawPixel(x0 - x, y0 + y);
    SSD1306_DrawPixel(x0 + x, y0 - y);
    SSD1306_DrawPixel(x0 - x, y0 - y);
    SSD1306_DrawPixel(x0 + y, y0 + x);
    SSD1306_DrawPixel(x0 - y, y0 + x);
    SSD1306_DrawPixel(x0 + y, y0 - x);
    SSD1306_DrawPixel(x0 - y, y0 - x);
  }

}

void SSD1306_DrawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername)
{
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      SSD1306_DrawPixel(x0 + x, y0 + y);
      SSD1306_DrawPixel(x0 + y, y0 + x);
    }
    if (cornername & 0x2) {
      SSD1306_DrawPixel(x0 + x, y0 - y);
      SSD1306_DrawPixel(x0 + y, y0 - x);
    }
    if (cornername & 0x8) {
      SSD1306_DrawPixel(x0 - y, y0 + x);
      SSD1306_DrawPixel(x0 - x, y0 + y);
    }
    if (cornername & 0x1) {
      SSD1306_DrawPixel(x0 - y, y0 - x);
      SSD1306_DrawPixel(x0 - x, y0 - y);
    }
  }

}

void SSD1306_FillCircle(int16_t x0, int16_t y0, int16_t r, int8_t color = TRUE)
{
  SSD1306_DrawFastVLine(x0, y0 - r, 2 * r + 1, color);
  SSD1306_FillCircleHelper(x0, y0, r, 3, 0, color);
}

// Used to do circles and roundrects
void SSD1306_FillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, int8_t color = TRUE) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x01) {
      SSD1306_DrawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
      SSD1306_DrawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
    }
    if (cornername & 0x02) {
      SSD1306_DrawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
      SSD1306_DrawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
    }
  }

}

// Draw a rectangle
void SSD1306_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
  SSD1306_DrawFastHLine(x, y, w);
  SSD1306_DrawFastHLine(x, y + h - 1, w);
  SSD1306_DrawFastVLine(x, y, h);
  SSD1306_DrawFastVLine(x + w - 1, y, h);
}

// Draw a rounded rectangle
void SSD1306_DrawRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r)
{
  // smarter version
  SSD1306_DrawFastHLine(x + r, y, w - 2 * r); // Top
  SSD1306_DrawFastHLine(x + r, y + h - 1, w - 2 * r); // Bottom
  SSD1306_DrawFastVLine(x, y + r, h - 2 * r); // Left
  SSD1306_DrawFastVLine(x + w - 1, y + r, h - 2 * r); // Right
  // draw four corners
  SSD1306_DrawCircleHelper(x + r, y + r, r, 1);
  SSD1306_DrawCircleHelper(x + w - r - 1, y + r, r, 2);
  SSD1306_DrawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4);
  SSD1306_DrawCircleHelper(x + r, y + h - r - 1, r, 8);
}

// Fill a rounded rectangle
void SSD1306_FillRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, int8_t color = TRUE)
{
  // smarter version
  SSD1306_FillRect(x + r, y, w - 2 * r, h, color);
  // draw four corners
  SSD1306_FillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  SSD1306_FillCircleHelper(x + r        , y + r, r, 2, h - 2 * r - 1, color);
}

// Draw a triangle
void SSD1306_DrawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
  SSD1306_DrawLine(x0, y0, x1, y1);
  SSD1306_DrawLine(x1, y1, x2, y2);
  SSD1306_DrawLine(x2, y2, x0, y0);
}

// Fill a triangle
void SSD1306_FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int8_t color = TRUE)
{
  int16_t a, b, y, last;
  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    ssd1306_swap(y0, y1); ssd1306_swap(x0, x1);
  }
  if (y1 > y2) {
    ssd1306_swap(y2, y1); ssd1306_swap(x2, x1);
  }
  if (y0 > y1) {
    ssd1306_swap(y0, y1); ssd1306_swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    SSD1306_DrawFastHLine(a, y0, b - a + 1, color);
    return;
  }

  int16_t
  dx01 = x1 - x0,
  dy01 = y1 - y0,
  dx02 = x2 - x0,
  dy02 = y2 - y0,
  dx12 = x2 - x1,
  dy12 = y2 - y1;
  int32_t  sa   = 0, sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1 - 1; // Skip it

  for(y = y0; y <= last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) ssd1306_swap(a, b);
    SSD1306_DrawFastHLine(a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y <= y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) ssd1306_swap(a, b);
    SSD1306_DrawFastHLine(a, y, b - a + 1, color);
  }
}

// invert the display
void SSD1306_InvertDisplay(int8_t i)
{
  if (i)
    ssd1306_command(SSD1306_INVERTDISPLAY_);
  else
    ssd1306_command(SSD1306_NORMALDISPLAY);
}

void SSD1306_SetTextWrap(int8_t w)
{
  wrap = w;
}

void SSD1306_DrawChar(uint8_t x, uint8_t y, uint8_t c, uint8_t size = 1)
{
  SSD1306_GotoXY(x, y);
  SSD1306_TextSize(size);
  SSD1306_Print(c);
}

void SSD1306_DrawText(uint8_t x, uint8_t y, char *_text, uint8_t size = 1)
{
  SSD1306_GotoXY(x, y);
  SSD1306_TextSize(size);
  while(*_text != '\0')
    SSD1306_Print(*_text++);

}

// move cursor to position (x, y)
void SSD1306_GotoXY(uint8_t x, uint8_t y)
{
  if((x >= SSD1306_LCDWIDTH) || (y >= SSD1306_LCDHEIGHT))
    return;
  x_pos = x;
  y_pos = y;
}

// set text size
void SSD1306_TextSize(uint8_t t_size)
{
  if(t_size < 1)
    t_size = 1;
  text_size = t_size;
}

/* print single char
    \a  Set cursor position to upper left (0, 0)
    \b  Move back one position
    \n  Go to start of current line
    \r  Go to line below
*/
void SSD1306_Print(uint8_t c)
{
  int8_t _color;
  uint8_t i, j, line;
  
  if (c == ' ' && x_pos == 0 && wrap)
    return;
  if(c == '\a') {
    x_pos = y_pos = 0;
    return;
  }
  if( (c == '\b') && (x_pos >= text_size * 6) ) {
    x_pos -= text_size * 6;
    return;
  }
  if(c == '\r') {
    x_pos = 0;
    return;
  }
  if(c == '\n') {
    y_pos += text_size * 8;
    if((y_pos + text_size * 7) > SSD1306_LCDHEIGHT)
      y_pos = 0;
    return;
  }

  if((c < ' ') || (c > '~'))
    c = '?';
  
  for(i = 0; i < 5; i++ ) {
    if(c < 'S')
      line = font[(c - ' ') * 5 + i];
    else
      line = font2[(c - 'S') * 5 + i];
    
    for(j = 0; j < 7; j++, line >>= 1) {
      if(line & 0x01)
        _color = TRUE;
      else
        _color = FALSE;
      if(text_size == 1) SSD1306_DrawPixel(x_pos + i, y_pos + j, _color);
      else               SSD1306_FillRect(x_pos + (i * text_size), y_pos + (j * text_size), text_size, text_size, _color);
    }
  }

  SSD1306_FillRect(x_pos + (5 * text_size), y_pos, text_size, 7 * text_size, FALSE);
  
  x_pos += text_size * 6;

  if( x_pos > (SSD1306_LCDWIDTH + text_size * 6) )
    x_pos = SSD1306_LCDWIDTH;

  if (wrap && (x_pos + (text_size * 5)) > SSD1306_LCDWIDTH)
  {
    x_pos = 0;
    y_pos += text_size * 8;
    if((y_pos + text_size * 7) > SSD1306_LCDHEIGHT)
      y_pos = 0;
  }
}

// print custom char (dimension: 7x5 pixel)
void SSD1306_PutCustomC(rom uint8_t *c)
{
  int8_t _color;
  uint8_t i, j, line;
  
  for(i = 0; i < 5; i++ ) {
    line = c[i];

    for(j = 0; j < 7; j++, line >>= 1) {
      if(line & 0x01)
        _color = TRUE;
      else
        _color = FALSE;
      if(text_size == 1) SSD1306_DrawPixel(x_pos + i, y_pos + j, _color);
      else               SSD1306_FillRect(x_pos + (i * text_size), y_pos + (j * text_size), text_size, text_size, _color);
    }
  }

  SSD1306_FillRect(x_pos + (5 * text_size), y_pos, text_size, 7 * text_size, FALSE);

  x_pos += (text_size * 6);

  if( x_pos > (SSD1306_LCDWIDTH + text_size * 6) )
    x_pos = SSD1306_LCDWIDTH;

  if (wrap && (x_pos + (text_size * 5)) > SSD1306_LCDWIDTH)
  {
    x_pos = 0;
    y_pos += text_size * 8;
    if((y_pos + text_size * 7) > SSD1306_LCDHEIGHT)
      y_pos = 0;
  }
}

// draw BMP stored in ROM
void SSD1306_ROMBMP(uint8_t x, uint8_t y, rom uint8_t *bitmap, uint8_t w, uint8_t h)
{
  for( uint16_t i = 0; i < h/8; i++ )
  {    
    for( uint16_t j = 0; j < (uint16_t)w * 8; j++ )
    {      
      if( bit_test(bitmap[j/8 + i*w], j % 8) == 1 )
        SSD1306_DrawPixel(x + j/8, y + i*8 + (j % 8));
      else
        SSD1306_DrawPixel(x + j/8, y + i*8 + (j % 8), 0);  
    }
  }
}
#endif  /* WHOLE_BANANA */

// end of driver code.

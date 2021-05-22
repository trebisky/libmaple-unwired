/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2011, 2012 LeafLabs, LLC.
 * Copyright 2014 Google, Inc.
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

// tjt - I now call this init.c
// originally in unwired
// I also moved start_c in here 11-24-2020

/**
 * @file wirish/boards.cpp
 * @brief init() and board routines.
 *
 * This file is mostly interesting for the init() function, which
 * configures Flash, the core clocks, and a variety of other available
 * peripherals on the board so the rest of Wirish doesn't have to turn
 * things on before using them.
 *
 * Prior to returning, init() calls boardInit(), which allows boards
 * to perform any initialization they need to. This file includes a
 * weak no-op definition of boardInit(), so boards that don't need any
 * special initialization don't have to define their own.
 *
 * How init() works is chip-specific. See the boards_setup.cpp files
 * under e.g. wirish/stm32f1/, wirish/stmf32f2 for the details, but be
 * advised: their contents are unstable, and can/will change without
 * notice.
 */

#include "boards.h"

#include <libmaple/libmaple_types.h>
#include <libmaple/flash.h>
#include <libmaple/nvic.h>
#include <libmaple/systick.h>

#include "serial.h"
// #include "io.h"

void initialize_pin_map ( void );
void board_setup_gpio(void);

/*
 * These addresses are where usercode starts when a bootloader is
 * present. If no bootloader is present, the user NVIC usually starts
 * at the Flash base address, 0x08000000.
 */
#if defined(BOOTLOADER_maple)
#define USER_ADDR_ROM 0x08005000
#elif defined(BOOTLOADER_robotis)
#define USER_ADDR_ROM 0x08003000
#endif
#define USER_ADDR_RAM 0x20000C00

extern char __text_start__;

static void
setup_nvic(void)
{
#ifdef VECT_TAB_FLASH
    nvic_init(USER_ADDR_ROM, 0);
#elif defined VECT_TAB_RAM
    nvic_init(USER_ADDR_RAM, 0);
#elif defined VECT_TAB_BASE
    nvic_init((uint32)0x08000000, 0);
#elif defined VECT_TAB_ADDR
    // A numerically supplied value
    nvic_init((uint32)VECT_TAB_ADDR, 0);
#else
    // Use the __text_start__ value from the linker script; this
    // should be the start of the vector table.
    nvic_init((uint32)&__text_start__, 0);
#endif
}

void
board_setup_flash ( void )
{
    // Turn on as many Flash "go faster" features as
    // possible. flash_enable_features() just ignores any flags it
    // can't support.
    flash_enable_features(FLASH_PREFETCH | FLASH_ICACHE | FLASH_DCACHE);

    // FLASH_SAFE_WAIT_STATES is a hack that needs to go away.
    flash_set_latency(FLASH_SAFE_WAIT_STATES);
}

void board_reset_pll ( void );
void board_setup_clock_prescalers ( void );

extern rcc_pll_cfg w_board_pll_cfg;

static rcc_clk w_board_pll_in_clk = RCC_CLK_HSE;

void
board_setup_clocks ( void )
{
    // Turn on HSI. We'll switch to and run off of this while we're
    // setting up the main PLL.
    rcc_turn_on_clk(RCC_CLK_HSI);

    // Turn off and reset the clock subsystems we'll be using, as well
    // as the clock security subsystem (CSS). Note that resetting CFGR
    // to its default value of 0 implies a switch to HSI for SYSCLK.
    RCC_BASE->CFGR = 0x00000000;
    rcc_disable_css();
    rcc_turn_off_clk(RCC_CLK_PLL);
    if (w_board_pll_in_clk != RCC_CLK_HSI) {
        rcc_turn_off_clk(w_board_pll_in_clk);
    }

    // wirish::priv::board_reset_pll();
    board_reset_pll();

    // Clear clock readiness interrupt flags and turn off clock
    // readiness interrupts.
    RCC_BASE->CIR = 0x00000000;

    // Enable the PLL input clock, and wait until it's ready.
    rcc_turn_on_clk(w_board_pll_in_clk);
    while (!rcc_is_clk_ready(w_board_pll_in_clk))
        ;

    // Configure AHBx, APBx, etc. prescalers and the main PLL.
    // wirish::priv::board_setup_clock_prescalers();
    board_setup_clock_prescalers();

    // rcc_configure_pll(&wirish::priv::w_board_pll_cfg);
    rcc_configure_pll(&w_board_pll_cfg);

    // Enable the PLL, and wait until it's ready.
    rcc_turn_on_clk(RCC_CLK_PLL);
    while(!rcc_is_clk_ready(RCC_CLK_PLL))
        ;

    // Finally, switch to the now-ready PLL as the main clock source.
    rcc_switch_sysclk(RCC_CLKSRC_PLL);
}

static void
timer_default_config ( timer_dev *dev )
{
    timer_adv_reg_map *regs = (dev->regs).adv;
    const uint16 full_overflow = 0xFFFF;
    const uint16 half_duty = 0x8FFF;

    timer_init(dev);
    timer_pause(dev);

    regs->CR1 = TIMER_CR1_ARPE;
    regs->PSC = 1;
    regs->SR = 0;
    regs->DIER = 0;
    regs->EGR = TIMER_EGR_UG;
    switch (dev->type) {
    case TIMER_ADVANCED:
        regs->BDTR = TIMER_BDTR_MOE | TIMER_BDTR_LOCK_OFF;
        // fall-through
    case TIMER_GENERAL:
        timer_set_reload(dev, full_overflow);
        for (uint8 channel = 1; channel <= 4; channel++) {
            if (timer_has_cc_channel(dev, channel)) {
                timer_set_compare(dev, channel, half_duty);
                timer_oc_set_mode(dev, channel, TIMER_OC_MODE_PWM_1,
                                  TIMER_OC_PE);
            }
        }
        // fall-through
    case TIMER_BASIC:
        break;
    }

    timer_generate_update(dev);
    timer_resume(dev);
}

/* from below */
static void
board_setup_timers(void)
{
    timer_foreach ( timer_default_config );
}

extern adc_smp_rate w_adc_smp;
extern adc_prescaler w_adc_pre;

static void
adc_default_config ( const adc_dev *dev )
{
    adc_enable_single_swstart(dev);
    // adc_set_sample_rate(dev, wirish::priv::w_adc_smp);
    adc_set_sample_rate(dev, w_adc_smp);
}

/* from below */
static void
board_setup_adcs ( void )
{
    //adc_set_prescaler (wirish::priv::w_adc_pre);
    adc_set_prescaler ( w_adc_pre );

    adc_foreach(adc_default_config);
}

/* Also moved here from stm32f1_setup.c
 * just so we can see everything at a glance.
 */
static void
series_init ( void )
{
    // Initialize AFIO here, too, so peripheral remaps and external
    // interrupts work out of the box.
    afio_init();
}

extern void usb_serial_begin ();

void
init ( void )
{
    initialize_pin_map ();

    board_setup_flash();
    board_setup_clocks();

    setup_nvic();
    systick_init(SYSTICK_RELOAD_VAL);

    board_setup_gpio ();

    board_setup_adcs ();
    board_setup_timers ();

    console_init ();
    /* Moved here from stm32f1_setup.c so I can comment on it.
     * And since all board_setup_usb() does is to call
     * usb_serial_begin(), we just cut out all the middlemen,
     * more for clarity and one stop shopping to see it here.
     */
    // board_setup_usb ();

    // printf ( "In init ....\n" );
    usb_serial_begin ();
    // printf ( "USB serial begin has returned\n" );

    /* This simply calls afio_init().
     * And all that does is to gate on the AFIO clock
     * and reset AFIO.
     */
    series_init ();

    // All that boardInit() does is to disable the debug ports
    // (i.e. SWD, so let's see if that is the culprit in
    // my hassle with using SWD and my ST-link.
    // and it is !!!!  So keep this commented out. !!!!
    // boardInit();
}

/* The linker must ensure that these are at least 4-byte aligned. */
extern char __data_start__, __data_end__;
extern char __bss_start__, __bss_end__;

struct rom_img_cfg {
    int *img_start;
};

extern char _lm_rom_img_cfgp;

// void init ( void );
void main ( void );

void
__attribute__((noreturn))
start_c (void)
{
    struct rom_img_cfg *img_cfg = (struct rom_img_cfg*)&_lm_rom_img_cfgp;
    int *src = img_cfg->img_start;
    int *dst = (int*)&__data_start__;
    // int exit_code;

    /* Initialize .data, if necessary. */
    /* The first "if" looks totally bogus and broken, tjt */
    if (src != dst) {
        int *end = (int*)&__data_end__;
        while (dst < end) {
            *dst++ = *src++;
        }
    }

    /* Zero .bss. */
    dst = (int*)&__bss_start__;
    while (dst < (int*)&__bss_end__) {
        *dst++ = 0;
    }

#ifdef notdef
    /* Run C++ initializers. */
    __libc_init_array();
#endif

    init ();

    /* Jump to main. */
    // exit_code = main(0, 0, 0);
    main ();

    //if (exit) {
    //    exit(exit_code);
    //}

    /* If exit is NULL, make sure we don't return. */
    for (;;)
        ;
}

/* THE END */

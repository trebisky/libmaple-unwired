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

// #include <wirish/boards.h>
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

    printf ( "In init ....\n" );
    usb_serial_begin ();
    printf ( "USB serial begin has returned\n" );

    /* This simply calls afio_init().
     * And all that does it to gate on the AFIO clock
     * and reset AFIO.
     */
    series_init ();

    // All that boardInit() does is to disable the debug ports
    // (i.e. SWD, so let's see if that is the culprit in
    // my hassle with using SWD and my ST-link.
    // and it is !!!!  So keep this commented out. !!!!
    // boardInit();
}

#ifdef OLD_BOARDS_CPP

// #include "boards_private.h"
// I am just copying the "private" file in here verbatim, as it is all C++ mumbo jumbo.

void old_init_rubbish ()
{
    /*
    -- wirish::priv::board_setup_flash();
    -- wirish::priv::board_setup_clocks();
    -- setup_nvic();
    -- systick_init(SYSTICK_RELOAD_VAL);
    -- wirish::priv::board_setup_gpio();
    -- wirish::priv::board_setup_adcs();
    -- wirish::priv::board_setup_timers();
    -- wirish::priv::board_setup_usb();
    -- wirish::priv::series_init();
    -- boardInit();
    */
}

/**
 * @file wirish/boards_private.h
 * @author Marti Bolivar <mbolivar@leaflabs.com>
 * @brief Private board support header.
 *
 * This file declares chip-specific variables and functions which
 * determine how init() behaves. It is not part of the public Wirish
 * API, and can change without notice.
 */

#include <libmaple/rcc.h>
#include <libmaple/adc.h>

// tjt - this never seems to be used anywhere.
/* Makes the PIN_MAP rows more human-readable. */
#define PMAP_ROW(gpio_dev, gpio_bit, timer_dev, timer_ch, adc_dev, adc_ch) \
    { gpio_dev, timer_dev, adc_dev, gpio_bit, timer_ch, adc_ch }

namespace wirish {
    namespace priv {

        /*
         * Chip- and board-specific initialization data
         */

        extern rcc_pll_cfg w_board_pll_cfg;
        extern rcc_clk w_board_pll_in_clk;
        extern adc_prescaler w_adc_pre;
        extern adc_smp_rate w_adc_smp;

        /*
         * Chip- and board-specific initialization routines and helper
         * functions.
         *
         * Some of these have default (weak) implementations in
         * boards.cpp; define them in your board file to override.
         */

        void board_reset_pll(void);
        void board_setup_clock_prescalers(void);
        void board_setup_gpio(void);
        void board_setup_usb(void);
        void board_setup_flash(void);
        void board_setup_adcs(void);
        void board_setup_timers(void);
        void board_setup_clocks(void);
        void series_init(void);

    }
}

// tjt - The following code was in main.cxx
// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
 __attribute__(( constructor )) void premain() {
    init();
}

void init(void) {
    wirish::priv::board_setup_flash();
    wirish::priv::board_setup_clocks();
    setup_nvic();
    systick_init(SYSTICK_RELOAD_VAL);
    wirish::priv::board_setup_gpio();
    wirish::priv::board_setup_adcs();
    wirish::priv::board_setup_timers();
    wirish::priv::board_setup_usb();
    wirish::priv::series_init();
    boardInit();
}

/* Provide a default no-op boardInit(). */
__weak void boardInit(void) {
}

/* You could farm this out to the files in boards/ if e.g. it takes
 * too long to test on boards with lots of pins. */
bool boardUsesPin(uint8 pin) {
    for (int i = 0; i < BOARD_NR_USED_PINS; i++) {
        if (pin == boardUsedPins[i]) {
            return true;
        }
    }
    return false;
}

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

static void setup_nvic(void) {
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

/*
 * Default implementations for some board-specific routines.
 */

namespace wirish {
namespace priv {

__weak rcc_clk w_board_pll_in_clk = RCC_CLK_HSE;

__weak void board_setup_flash(void) {
    // Turn on as many Flash "go faster" features as
    // possible. flash_enable_features() just ignores any flags it
    // can't support.
    flash_enable_features(FLASH_PREFETCH | FLASH_ICACHE | FLASH_DCACHE);
    // FLASH_SAFE_WAIT_STATES is a hack that needs to go away.
    flash_set_latency(FLASH_SAFE_WAIT_STATES);
}

__weak void board_setup_clocks(void) {
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
    wirish::priv::board_reset_pll();
    // Clear clock readiness interrupt flags and turn off clock
    // readiness interrupts.
    RCC_BASE->CIR = 0x00000000;

    // Enable the PLL input clock, and wait until it's ready.
    rcc_turn_on_clk(w_board_pll_in_clk);
    while (!rcc_is_clk_ready(w_board_pll_in_clk))
        ;

    // Configure AHBx, APBx, etc. prescalers and the main PLL.
    wirish::priv::board_setup_clock_prescalers();
    rcc_configure_pll(&wirish::priv::w_board_pll_cfg);

    // Enable the PLL, and wait until it's ready.
    rcc_turn_on_clk(RCC_CLK_PLL);
    while(!rcc_is_clk_ready(RCC_CLK_PLL))
        ;

    // Finally, switch to the now-ready PLL as the main clock source.
    rcc_switch_sysclk(RCC_CLKSRC_PLL);
}

static void adc_default_config(const adc_dev *dev) {
    adc_enable_single_swstart(dev);
    adc_set_sample_rate(dev, wirish::priv::w_adc_smp);
}

__weak void board_setup_adcs(void) {
    adc_set_prescaler(wirish::priv::w_adc_pre);
    adc_foreach(adc_default_config);
}

static void timer_default_config(timer_dev *dev) {
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

__weak void board_setup_timers(void) {
    timer_foreach(timer_default_config);
}

} /* end of namespace wirish */
} /* end of namespace priv */

#endif /* OLD_BOARDS_CPP */

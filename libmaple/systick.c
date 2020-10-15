/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2010, 2011 LeafLabs, LLC.
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

/**
 * @file libmaple/systick.c
 * @brief System timer (SysTick).
 */

#include <libmaple/systick.h>

/** SysTick register map type */
typedef struct systick_reg_map {
    __io uint32 CSR;            /**< Control and status register */
    __io uint32 RVR;            /**< Reload value register */
    __io uint32 CNT;            /**< Current value register ("count") */
    __io uint32 CVR;            /**< Calibration value register */
} systick_reg_map;

/** SysTick register map base pointer */
#define SYSTICK_BASE                    ((struct systick_reg_map*)0xE000E010)

/*
 * Register bit definitions.
 */

/* Control and status register */

#define SYSTICK_CSR_COUNTFLAG           BIT(16)
#define SYSTICK_CSR_CLKSOURCE           BIT(2)
#define SYSTICK_CSR_CLKSOURCE_EXTERNAL  0
#define SYSTICK_CSR_CLKSOURCE_CORE      BIT(2)
#define SYSTICK_CSR_TICKINT             BIT(1)
#define SYSTICK_CSR_TICKINT_PEND        BIT(1)
#define SYSTICK_CSR_TICKINT_NO_PEND     0
#define SYSTICK_CSR_ENABLE              BIT(0)
#define SYSTICK_CSR_ENABLE_MULTISHOT    BIT(0)
#define SYSTICK_CSR_ENABLE_DISABLED     0

/* Calibration value register */

#define SYSTICK_CVR_NOREF               BIT(31)
#define SYSTICK_CVR_SKEW                BIT(30)
#define SYSTICK_CVR_TENMS               0xFFFFFF

volatile uint32 systick_uptime_millis;
static void (*systick_user_callback)(void);

/**
 * @brief Initialize and enable SysTick.
 *
 * Clocks the system timer with the core clock, turns it on, and
 * enables interrupts.
 *
 * @param reload_val Appropriate reload counter to tick every 1 ms.
 */
void systick_init(uint32 reload_val) {
    SYSTICK_BASE->RVR = reload_val;
    systick_enable();
}

/**
 * Clock the system timer with the core clock, but don't turn it
 * on or enable interrupt.
 */
void systick_disable() {
    SYSTICK_BASE->CSR = SYSTICK_CSR_CLKSOURCE_CORE;
}

/**
 * Clock the system timer with the core clock and turn it on;
 * interrupt every 1 ms, for systick_timer_millis.
 */
void systick_enable() {
    /* re-enables init registers without changing reload val */
    SYSTICK_BASE->CSR = (SYSTICK_CSR_CLKSOURCE_CORE   |
                         SYSTICK_CSR_ENABLE           |
                         SYSTICK_CSR_TICKINT_PEND);
}

/**
 * @brief Attach a callback to be called from the SysTick exception handler.
 *
 * To detach a callback, call this function again with a null argument.
 */
void systick_attach_callback(void (*callback)(void)) {
    systick_user_callback = callback;
}

/**
 * @brief Returns the current value of the SysTick counter.
 */
uint32 systick_get_count(void) {
    return SYSTICK_BASE->CNT;
}

/**
 * @brief Check for underflow.
 *
 * This function returns 1 if the SysTick timer has counted to 0 since
 * the last time it was called.  However, any reads of any part of the
 * SysTick Control and Status Register SYSTICK_BASE->CSR will
 * interfere with this functionality.  See the ARM Cortex M3 Technical
 * Reference Manual for more details (e.g. Table 8-3 in revision r1p1).
 */
uint32 systick_check_underflow(void) {
    return SYSTICK_BASE->CSR & SYSTICK_CSR_COUNTFLAG;
}

/*
 * SysTick ISR
 */

void __exc_systick(void) {
    systick_uptime_millis++;
    if (systick_user_callback) {
        systick_user_callback();
    }
}

/* THE END */

/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
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
 * @file libmaple/include/libmaple/systick.h
 * @brief System timer definitions
 */

#ifndef _LIBMAPLE_SYSTICK_H_
#define _LIBMAPLE_SYSTICK_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <libmaple/libmaple_types.h>
#include <libmaple/util.h>

/** System elapsed time, in milliseconds */
extern volatile uint32 systick_uptime_millis;

/**
 * @brief Returns the system uptime, in milliseconds.
 */
static inline uint32 systick_uptime(void) {
    return systick_uptime_millis;
}

void systick_init(uint32 reload_val);
void systick_disable();
void systick_enable();
void systick_attach_callback(void (*)(void));

uint32 systick_get_count(void);
uint32 systick_check_underflow(void);

#ifdef notdef
/* tjt - no longer inline.
 */
/**
 * @brief Returns the current value of the SysTick counter.
 */
static inline uint32 systick_get_count(void) {
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
static inline uint32 systick_check_underflow(void) {
    return SYSTICK_BASE->CSR & SYSTICK_CSR_COUNTFLAG;
}
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
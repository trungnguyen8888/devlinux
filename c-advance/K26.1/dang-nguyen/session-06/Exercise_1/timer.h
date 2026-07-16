#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef void (*timer_callback_t)(void);

/**
 * @brief Register a callback to fire after a given number of ticks.
 * @param[in] expire_at_tick  Tick number at which the callback fires.
 * @param[in] callback        Function pointer to invoke. Must not be NULL.
 */
// NOLINTNEXTLINE(readability-identifier-naming)
void timer_register(const uint32_t expire_at_tick, const timer_callback_t callback);

/**
 * @brief Advance the timer by one tick. Fires callback if expired.
 */
void timer_tick(void);

/**
 * @brief Cancel any active timer.
 */
void timer_reset(void);

#endif
#include <stdint.h>
#include <stdio.h>
#include "timer.h"

#define FIRST_FIRING_TICK   (5U)
#define SECOND_FIRING_TICK  (3U)
#define TICK_NUM            (10U)

#define APP_SUCCESS         (0)

void my_alarm(void);
void my_second_alarm(void);

uint32_t alarm_point = 0U;

void my_alarm(void)
{
    printf("[ALARM] Timer fired at tick %u!\n", alarm_point);
}

void my_second_alarm(void)
{
    printf("[ALARM] Second alarm fired at tick %u!\n", alarm_point);
}

int32_t main(void)
{
    uint32_t expire_at_tick = FIRST_FIRING_TICK;

    printf("--- Test 1: Alarm at tick %u, run for %u ticks ---\n", expire_at_tick, TICK_NUM);

    timer_register(expire_at_tick, &my_alarm);

    for (uint32_t loop = 0U; loop < TICK_NUM; loop++)
    {
        alarm_point = loop + 1U;

        timer_tick();

        // NOLINTNEXTLINE(readability-magic-numbers)
        if (5U == loop)
        {
            timer_register(expire_at_tick, &my_alarm);
        }
    }

    printf("\n");
    expire_at_tick = SECOND_FIRING_TICK;
    printf("--- Test 2: Reset, then new alarm at tick %u ---\n", expire_at_tick);

    timer_reset();

    timer_register(expire_at_tick, &my_second_alarm);

    // NOLINTNEXTLINE(readability-magic-numbers)
    for (uint32_t loop = 0U; loop < 5U; loop++)
    {
        alarm_point = loop + 1U;
        timer_tick();
    }

    return APP_SUCCESS;
}
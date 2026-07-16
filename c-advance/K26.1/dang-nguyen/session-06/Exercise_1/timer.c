#include "timer.h"

typedef struct st_timer
{
    uint32_t         expire_at_tick; /**< Tick count at which the callback fires. */
    uint32_t         current_tick;   /**< Current elapsed tick count.             */
    timer_callback_t on_expire;      /**< Callback to invoke on expiry.           */
    bool             is_running;     /**< True if a timer is currently active.    */
} st_timer_t;

/**
 * @brief Query whether a timer is currently active.
 * @return true if a timer is running, false otherwise.
 */
static inline bool timer_is_running(void);

/* Internal timer state — static, invisible to other modules */
static st_timer_t my_timer =
{
    .expire_at_tick = 0U,
    .current_tick   = 0U,
    .on_expire      = NULL,
    .is_running     = false
};

static inline bool timer_is_running(void)
{
    return my_timer.is_running;
}

// NOLINTNEXTLINE(readability-identifier-naming)
void timer_register(const uint32_t expire_at_tick, const timer_callback_t callback)
{
    if (0U == expire_at_tick)
    {
        printf("[ERROR] %s: expire_at_tick is 0!\n", __func__);
    }
    else if (NULL == callback)
    {
        printf("[ERROR] %s: callback is NULL!\n", __func__);
    }
    else if (true == timer_is_running())
    {
        printf("[WARN] Timer already running! Ignoring new registration.\n");
    }
    else
    {
        my_timer.expire_at_tick = expire_at_tick;
        my_timer.on_expire      = callback;
        my_timer.current_tick   = 0U;
        my_timer.is_running     = true;
    }
}

void timer_tick(void)
{
    if (true == timer_is_running())
    {
        my_timer.current_tick++;

        printf("Tick %u...\n", my_timer.current_tick);
    
        if (my_timer.current_tick == my_timer.expire_at_tick)
        {
            if (NULL != my_timer.on_expire)
            {
                my_timer.on_expire();
            }

            my_timer.is_running = false;
        }

    }
}

void timer_reset(void)
{
    printf("[TIMER] Reset.\n");

    my_timer.current_tick   = 0U;
    my_timer.expire_at_tick = 0U;
    my_timer.on_expire      = NULL;
    my_timer.is_running     = false;
}
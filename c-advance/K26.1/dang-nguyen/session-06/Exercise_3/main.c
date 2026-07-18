#include <stdint.h>
#include "cmd_dispatcher.h"

#define ARRAY_SIZE(a)   (uint32_t)(sizeof(a) / sizeof((a)[0]))

#define APP_SUCCESS     (0)

int32_t main(void)
{
    static const char *p_cmd_str[] =
    {
        "LED_ON",
        "LED_OFF",
        "MOTOR_START",
        "MOTOR_STOP",
        "STATUS",
        "DUMMY"
    };

    for (uint32_t i = 0U; i < ARRAY_SIZE(p_cmd_str); i++)
    {
        dispatch_command(p_cmd_str[i]);
    }

    return APP_SUCCESS;
}
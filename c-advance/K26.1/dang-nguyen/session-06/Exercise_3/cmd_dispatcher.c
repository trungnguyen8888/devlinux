#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cmd_dispatcher.h"

/**
 * @brief Calculate the number of elements in a statically allocated array.
 *
 * @param[in] a Array whose number of elements is calculated.
 */
#define ARRAY_SIZE(a)   (uint32_t)(sizeof(a) / sizeof((a)[0]))

/**
 * @brief Command handler function pointer type.
 *
 * Represents a command action that takes no parameters and returns no value.
 */
typedef void (*cmd_action_t)(void);

/**
 * @brief Store a command keyword and its associated handler function.
 */
typedef struct st_cmd_entry
{
    const char   *p_cmd_str; /**< Command keyword string. */
    cmd_action_t  action;    /**< Handler function.       */
} st_cmd_entry_t;

/**
 * @brief Turn on the LED.
 */
static void led_on(void);

/**
 * @brief Turn off the LED.
 */
static void led_off(void);

/**
 * @brief Start the motor.
 */
static void motor_start(void);

/**
 * @brief Stop the motor.
 */
static void motor_stop(void);

/**
 * @brief Display the current system status.
 */
static void status(void);

/**
 * @brief Command dispatch table.
 *
 * Maps each supported command keyword to its corresponding handler function.
 */
static const st_cmd_entry_t cmd_table[] =
{
    { .p_cmd_str = "LED_ON",      .action = &led_on      }, /**< 0 */
    { .p_cmd_str = "LED_OFF",     .action = &led_off     }, /**< 1 */
    { .p_cmd_str = "MOTOR_START", .action = &motor_start }, /**< 2 */
    { .p_cmd_str = "MOTOR_STOP",  .action = &motor_stop  }, /**< 3 */
    { .p_cmd_str = "STATUS",      .action = &status      }  /**< 4 */
};

static void led_on(void)
{
    printf("[CMD] LED turned ON.\n");
}

static void led_off(void)
{
    printf("[CMD] LED turned OFF.\n");
}

static void motor_start(void)
{
    printf("[CMD] Motor started at 1500 RPM.\n");
}

static void motor_stop(void)
{
    printf("[CMD] Motor stopped.\n");
}

static void status(void)
{
    printf("[CMD] System status: OK.\n");
}

/**
 * @brief Dispatch a received command to its registered handler.
 */
void dispatch_command(const char *const p_received_cmd)
{
    if ((NULL == p_received_cmd) || ('\0' == *p_received_cmd))
    {
        printf("[ERROR] Invalid command string!\n");
    }
    else
    {
        uint32_t cmd_idx = 0U;

        while (cmd_idx < ARRAY_SIZE(cmd_table))
        {
            if (0 == strcmp(p_received_cmd, cmd_table[cmd_idx].p_cmd_str))
            {
                cmd_table[cmd_idx].action();
                break;
            }

            /* Continue with the next entry if the command does not match. */
            cmd_idx++;
        }

        /* Report an unsupported command if no matching entry was found. */
        if (ARRAY_SIZE(cmd_table) == cmd_idx)
        {
            printf("Unknown command!\n");
        }
    }
}
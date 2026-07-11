#ifndef CONSOLE_DISPLAY_H
#define CONSOLE_DISPLAY_H

#include "i_display.h"

/**
 * @brief Create and return opaque handle to console display configuration.
 * 
 * Initializes static storage with specified baud_rate.
 * The returned pointer is valid for the entire program lifetime.
 * 
 * @param[in] baud_rate UART baud rate configuration.
 * @return Opaque handle to configuration (never NULL).
 */
display_config_handle_t console_config_create(uint32_t baud_rate);

extern const st_i_display_t console_display;

#endif
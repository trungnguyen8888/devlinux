#ifndef DUMMY_DISPLAY_H
#define DUMMY_DISPLAY_H

#include "i_display.h"

/**
 * @brief Get the number of dummy display draw operations.
 * 
 * Returns the number of times dummy_display_draw_pixel() has been called
 * since the dummy display was initialized.
 * 
 * @return Number of draw operations performed by the dummy display.
 */
uint32_t get_draw_count(void);

extern const st_i_display_t dummy_display;

#endif
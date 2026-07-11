#ifndef I_DISPLAY_H
#define I_DISPLAY_H

#include <stdio.h>
#include <stdint.h>

/**
 * @brief Display configuration handle.
 */
typedef struct st_display_config    *display_config_handle_t;

/**
 * @brief Display driver error codes.
 */
typedef enum e_errcode {
    DL_RET_OK            = 0U,  /**< Operation successful */
    DL_RET_INVALID_PARAM = 1U   /**< Invalid parameter provided */
} e_errcode_t;

/**
 * @brief Display driver interface (HAL).
 * Contains function pointers for polymorphic display operations.
 */
typedef struct st_i_display
{
    /** @brief Initialize display with configuration */
    e_errcode_t (*init)(display_config_handle_t p_cfg_hdl);

    /** @brief Draw pixel at (x, y) with given colour */
    void (*draw_pixel)(const uint16_t x, const uint16_t y, const uint8_t colour);
} st_i_display_t;

#endif
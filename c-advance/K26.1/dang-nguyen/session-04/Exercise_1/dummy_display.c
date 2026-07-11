#include "dummy_display.h"

static e_errcode_t dummy_display_init(const display_config_handle_t p_cfg_hdl);
static void dummy_display_draw_pixel(const uint16_t x, const uint16_t y, const uint8_t colour);

const st_i_display_t dummy_display = {
    .init       = &dummy_display_init,
    .draw_pixel = &dummy_display_draw_pixel
};

static uint32_t dummy_draw_count;

static e_errcode_t dummy_display_init(const display_config_handle_t p_cfg_hdl)
{
    (void)p_cfg_hdl;
    dummy_draw_count = 0U;

    return DL_RET_OK;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void dummy_display_draw_pixel(const uint16_t x, const uint16_t y, const uint8_t colour)
{
    /* Unused */
    (void)x;
    (void)y;
    (void)colour;

    /* Count the draw times */
    dummy_draw_count++;
}

uint32_t get_draw_count(void)
{
    return dummy_draw_count;
}
#include "console_display.h"
#include "dummy_display.h"

#define BAUD_RATE   (10U)
#define PIXEL_NUM   (4U)

#define APP_SUCCESS (0U)
#define APP_FAILURE (1U)

typedef struct st_pixel_info
{
    uint16_t x;
    uint16_t y;
    uint8_t  colour;
} st_pixel_info_t;

typedef enum e_display_type
{
    CONSOLE = 0U,
    DUMMY,
    TYPE_COUNT
} e_display_type_t;

static int32_t draw_rectangle(
    const st_i_display_t    *const p_disp
);

static int32_t process(
    const st_i_display_t            *const p_type,
    const e_display_type_t          type_id,
    const display_config_handle_t   p_hdl
);

static const char *p_type_name[TYPE_COUNT] =
{
    [CONSOLE] = "Console",
    [DUMMY]   = "Dummy"
};

static int32_t draw_rectangle(
    const st_i_display_t    *const p_disp
)
{
    static st_pixel_info_t pixel_info[PIXEL_NUM] =
    {
        [0] = { .x = 0, .y = 0, .colour = 1 },
        [1] = { .x = 1, .y = 0, .colour = 1 },
        [2] = { .x = 0, .y = 1, .colour = 1 },
        [3] = { .x = 1, .y = 1, .colour = 1 }
    };
    int32_t ret = APP_SUCCESS;

    if (NULL == p_disp)
    {
        printf("[%s]: p_disp is NULL!\n", __func__);
        ret = APP_FAILURE;
    }
    else if (NULL == p_disp->draw_pixel)
    {
        printf("[%s]: p_disp->draw_pixel is NULL!\n", __func__);
        ret = APP_FAILURE;
    }
    else
    {
        for (uint32_t i = 0U; i < PIXEL_NUM; i++)
        {
            p_disp->draw_pixel(pixel_info[i].x, pixel_info[i].y, pixel_info[i].colour);
        }
    }

    return ret;
}

static int32_t process(
    const st_i_display_t            *const p_type,
    const e_display_type_t          type_id,
    const display_config_handle_t   p_hdl
)
{
    int32_t ret = APP_SUCCESS;
    e_errcode_t api_ret = DL_RET_OK;

    if (TYPE_COUNT <= type_id)
    {
        printf("[%s]: Invalid display type!\n", __func__);
        ret = APP_FAILURE;
    }
    else if (NULL == p_type)
    {
        printf("[%s]: p_type is NULL!\n", __func__);
        ret = APP_FAILURE;
    }
    else if (NULL == p_type->init)
    {
        printf("[%s]: %s's Init( ) is not set!\n", __func__, p_type_name[type_id]);
        ret = APP_FAILURE;
    }
    else
    {
        api_ret = p_type->init(p_hdl);
    }

    if (APP_SUCCESS == ret)
    {
        if (DL_RET_OK == api_ret)
        {
            ret = draw_rectangle(p_type);
        }
        else
        {
            ret = APP_FAILURE;
        }
    }

    return ret;
}

int32_t main(void)
{
    display_config_handle_t p_dpl_handle = NULL;
    int32_t ret = APP_SUCCESS;

    p_dpl_handle = console_config_create(BAUD_RATE);

    ret = process(&console_display, CONSOLE, p_dpl_handle);

    if (APP_SUCCESS == ret)
    {
        ret = process(&dummy_display, DUMMY, p_dpl_handle);
        printf("Dummy display was called %u times.\n", get_draw_count());
    }

    return ret;
}
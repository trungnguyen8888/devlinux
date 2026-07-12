/*
 * 1. Look at the bss value. The pool array packet_pool[5] and packet_in_use[5] are both
 * static variables with no explicit initialiser. Which memory section do they live in
 * (.text, .data, or .bss) and why?
 * 
 * -> Answer: They live in the .bss because the .bss section is used for global and static
 * variables which are uninitialised or whose initial values are all 0. The startup code
 * reserves RAM for such variables and clear that memory to zero.
 * 
 * 2. If you changed the pool declaration to initialise it explicitly:
 *  static st_network_packet_t packet_pool[5] = {0};
 * Would it move from .bss to .data? Why or why not? (Hint: the C standard says 0-initialised
 * static storage is equivalent to uninitialized static storage.)
 * 
 * -> Answer: No, they still reside in the .bass section. Because the .bss section stores
 * global and static variables which are uninitialised or whose initial values are all 0.
 * static st_network_packet_t packet_pool[5] = {0} initialises packet_pool[0] to 0 and the
 * leaves the over elements 0.
 * 
 * 3. The packet_alloc() and packet_free() function bodies (compiled machine code) live in
 * which memory section? On a real MCU with 64KB Flash and 16KB RAM, where would each section
 * physically reside?
 * 
 * -> Answer: They live in .text section. On an MCU with 64KB of Flash and 16KB of RAM, the
 * sections would normally be located as follow:
 *  .text | Flash | Compiled machine code and often read-only constants
 *  .data | RAM (in runtime) | Global and static variables initialized to non-zero values
 *  .bss | RAM | Global and static variables initialized to zero
 * 
 * The initial values for .data must also be stored somewhere in Flash. During startup, the
 * startup code copies those initial values from Flash into the .data region in RAM.
 * 
 * The .bss section does not require a full initial-value image in Flash. During startup,
 * the startup code clears its assigned RAM region to zero.
 * 
 * A simplified startup sequence is:
 *  I.  Copy the initial values of .data from Flash to RAM.
 *  II. Clear the .bss region in RAM to zero.
 *  III.Call main().
 * 
 * 4. If POOL_SIZE was increased from 5 to 50, which section grows and by how much? Calculate
 * the expected new .bss size in bytes, given sizeof(st_network_packet_t) = 68 bytes.
 * 
 * -> Answer: Current POOL_SIZE is 5U -> Total size of packet_pool is 5 * 68 = 340 bytes.
 * If POOL_SIZE was 50U -> .bss's size increases by (50 - 5) * 68 = 3060 bytes.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "packet_util.h"

#define PACKET_NUM      (5U)
#define PACKET_TEST_IDX (6U)

#define APP_SUCCESS     (0)
#define APP_FAILURE     (1)

static int32_t process(const st_packet_util_t *const p_hal_obj);

packet_handle_t p_handle[PACKET_NUM] = { NULL, NULL, NULL, NULL, NULL };

static int32_t process(const st_packet_util_t *const p_hal_obj)
{
    int32_t ret = APP_SUCCESS;

    if (NULL == p_hal_obj)
    {
        printf("[ERROR] HAL obj is NULL!\n");
        ret = APP_FAILURE;
    }
    else if ((NULL == p_hal_obj->Allocate) || (NULL == p_hal_obj->Free))
    {
        printf("[ERROR] HAL obj is not initialized\n");
        ret = APP_FAILURE;
    }
    else
    {
        uint8_t idx = 0U;
        
        /* Allocate all 5 packets */
        for (idx = 0U; idx < PACKET_NUM; idx++)
        {
            p_handle[idx] = p_hal_obj->Allocate();
            
            if (NULL != p_handle[idx])
            {
                printf("Allocating packet %u: Success!\n", (idx + 1U));
            }
            else
            {
                printf("Allocating packet %u: Failed (Pool Full)\n", (idx + 1U));
            }
        }

        /* Try allocating the 6th packet */
        if (p_hal_obj->Allocate() != NULL)
        {
            printf("Allocating packet %u: Success!\n", PACKET_TEST_IDX);
        }
        else
        {
            printf("Allocating packet %u: Failed (Pool Full)\n", PACKET_TEST_IDX);
        }

        /* Free one packet */
        printf("Freeing packet 2...\n");
        p_hal_obj->Free(p_handle[2]);
        p_handle[2] = NULL;

        /* Try allocating the 6th packet again */
        p_handle[2] = p_hal_obj->Allocate();
        if (NULL != p_handle[2])
        {
            printf("Allocating packet %u: Success!\n", PACKET_TEST_IDX);
        }
        else
        {
            printf("Allocating packet %u: Failed (Pool Full)\n", PACKET_TEST_IDX);
        }
    }

    return ret;
}

int32_t main(void)
{
    int32_t ret = APP_SUCCESS;

    /* Get HAL object */
    st_packet_util_t *p_hal_obj = NULL;

    /* Initialize the system */
    init_sys((void **)&p_hal_obj);

    if (NULL == p_hal_obj)
    {
        printf("[ERROR] Cannot initialize system!\n");
        ret = APP_FAILURE;
    }
    else
    {
        ret = process(p_hal_obj);
    }

    return ret;
}
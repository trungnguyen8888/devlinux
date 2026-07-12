#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "packet_util.h"

#define POOL_SIZE       (5U)
#define PAYLOAD_SIZE    (64U)

typedef struct st_network_packet {
    uint32_t id;
    uint8_t payload[PAYLOAD_SIZE];
} st_network_packet_t;

static st_network_packet_t packet_pool[POOL_SIZE];
static bool packet_in_use[POOL_SIZE];

/**
 * @brief Initialize the object pool (mark all as free)
 */
static void pool_init(void);

/**
 * @brief Allocate a packet from the pool.
 * @return Pointer to packet, or NULL if pool is full.
 */
static packet_handle_t packet_alloc(void);

/**
 * @brief Free a packet back to the pool.
 * @param p_packet Pointer to the packet to free.
 */
static void packet_free(const packet_handle_t p_packet);

static void pool_init(void)
{
    for (uint32_t idx = 0U; idx < POOL_SIZE; idx++)
    {
        packet_pool[idx].id = idx;
        packet_in_use[idx]  = false;
    }
}

static packet_handle_t packet_alloc(void)
{
    st_network_packet_t *p_avail_pkt = NULL;

    for (uint32_t idx = 0U; idx < POOL_SIZE; idx++)
    {
        if (false == packet_in_use[idx])
        {
            packet_in_use[idx] = true;
            p_avail_pkt = &packet_pool[idx];
            break;
        }
    }

    return p_avail_pkt;
}

void packet_free(const packet_handle_t p_packet)
{
    if (NULL != p_packet)
    {
        for (uint32_t idx = 0U; idx < POOL_SIZE; idx++)
        {
            if (p_packet == &packet_pool[idx])
            {
                packet_in_use[idx] = false;
                break;
            }
        }
    }
    else
    {
        printf("[ERROR] %s: Invalid packet!\n", __func__);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
void init_sys(void **const pp_hal_obj)
{
    static st_packet_util_t hal_obj;

    /* Initialize the pool */
    pool_init();

    /* Initialize HAL object */
    hal_obj.Allocate = &packet_alloc;
    hal_obj.Free     = &packet_free;

    /* Get HAL object */
    *pp_hal_obj = (void *)&hal_obj;
}
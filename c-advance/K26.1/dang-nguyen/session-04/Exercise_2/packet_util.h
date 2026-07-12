#ifndef PACKET_UTIL_H
#define PACKET_UTIL_H

/**
 * @brief Network packet handle.
 */
typedef struct st_network_packet    *packet_handle_t;

/**
 * @brief Network packet interface (HAL).
 * Contains function pointers for polymorphic packet operations.
 */
typedef struct st_packet_util
{
    /**
     * @brief Allocate a packet from the pool.
     * @return Pointer to packet, or NULL if pool is full.
     */
    packet_handle_t (*Allocate)(void);

    /**
     * @brief Free a packet back to the pool.
     * @param p_packet Pointer to the packet to free.
     */
    void (*Free)(const packet_handle_t p_packet);
} st_packet_util_t;

/**
 * @brief Create and return opaque handle to console display configuration.
 * 
 * Initializes static storage with specified baud_rate.
 * The returned pointer is valid for the entire program lifetime.
 * 
 * @param[in] baud_rate UART baud rate configuration.
 * @return Opaque handle to configuration (never NULL).
 */
// NOLINTNEXTLINE(readability-identifier-naming)
void init_sys(void **const pp_hal_obj);

#endif
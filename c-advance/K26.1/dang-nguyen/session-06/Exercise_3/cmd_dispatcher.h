#ifndef CMD_DISPATCHER_H
#define CMD_DISPATCHER_H

/**
 * @brief Dispatch a received ASCII command string to its handler.
 * @param[in] p_received_cmd  Null-terminated command string. Must not be NULL.
 */
void dispatch_command(const char *const p_received_cmd);

#endif
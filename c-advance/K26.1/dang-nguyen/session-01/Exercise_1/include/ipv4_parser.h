/*********************************************************************************************
 Start of file
 ********************************************************************************************/
/**
 * @file ipv4_parser.h
 * @brief IPv4 Parser library
 */
#ifndef IPV4_PARSER_H
#define IPV4_PARSER_H

/*********************************************************************************************
 Includes
 ********************************************************************************************/
#include <stdint.h>
#include "ipv4_types.h"

/*********************************************************************************************
 Public function declarations
 ********************************************************************************************/
/**
 * @brief Parse an IPv4 address string into a 32-bit unsigned integer.
 *
 * This function validates and parses an IPv4 address string in dotted-decimal
 * format. The parsed IPv4 value is stored as a 32-bit unsigned integer.
 *
 * For example:
 *  "192.168.1.50" -> 0xC0A80132
 *
 * @param[in]  p_ip_str  Null-terminated IPv4 string to be parsed.
 *                       Expected format: "a.b.c.d", where each octet is in
 *                       the range 0 to 255.
 * @param[out] p_ip_out  Pointer to store the parsed 32-bit IPv4 value.
 *
 * @return e_errcode_t.
 *
 * @retval EC_RET_SUCCESS       Normal operation.
 * @retval EC_RET_ARG_NULL      Null pointer input:
 *                              - p_ip_out is NULL.
 *                              - p_ip_str is NULL.
 * @retval EC_RET_INVALID_PARAM Invalid IPv4 string:
 *                              - IP string is empty.
 *                              - IP string starts with a non-number character.
 *                              - IP string ends with a non-number character.
 *                              - An octet is missing.
 *                              - An octet has more than 3 digits.
 *                              - An octet value is greater than 255.
 *                              - Octets are not delimited by dots.
 *                              - The IP string has unwanted trailing characters.
 */
e_errcode_t parse_ipv4(
    const char  *p_ip_str,
    uint32_t    *const p_ip_out
);

#endif  /**< IPV4_PARSER_H */

/*********************************************************************************************
 End of file
 ********************************************************************************************/
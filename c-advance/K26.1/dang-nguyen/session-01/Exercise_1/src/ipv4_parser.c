/*********************************************************************************************
 Start of file
 ********************************************************************************************/
/*********************************************************************************************
 Includes
 ********************************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ipv4_parser.h"

/*********************************************************************************************
 Macro definitions
 ********************************************************************************************/
/* IPv4 dotted-decimal format constraints */
#define IPV4_OCTET_DIGIT_MAX    (3U)    /**< Maximum decimal digits in one octet */
#define IPV4_OCTET_COUNT        (4U)    /**< Number of octets in an IPv4 address */
#define IPV4_OCTET_MAX          (255U)  /**< Maximum numeric value of one octet  */

/* Numeric conversion constants */
#define DECIMAL_BASE            (10U)   /**< Radix used for decimal accumulation */
#define BIT_NUM_PER_BYTE        (8U)    /**< Bit width of one IPv4 octet         */

/* Basic validation helpers */
#define IS_INVALID(ptr)         (NULL == (ptr))                 /**< Check whether a pointer is NULL  */
#define IS_NUM(c)               (('0' <= (c)) && ((c) <= '9'))  /**< Check for an ASCII decimal digit */
#define IS_NOT_NUM(c)           (!IS_NUM(c))                    /**< Check for a non-decimal digit    */

/* Macros for debugging */
#if defined(DEBUG)
#define DBG_LOG(...)                 \
    do                               \
    {                                \
        printf("%s: ", __func__);    \
        printf(__VA_ARGS__);         \
        printf("\n");                \
    } while (0)
#else
#define DBG_LOG(...)                 \
    do                               \
    {                                \
    } while (0)
#endif

/*********************************************************************************************
 Private function declarations
 ********************************************************************************************/
static e_errcode_t convert_octet(
    const char      **pp_str,           /* NOLINT(readability-identifier-naming) */
    uint32_t        *const p_octet
);

static e_errcode_t move_to_next_octet(
    const uint8_t   idx,
    const char      **pp_str            /* NOLINT(readability-identifier-naming) */
);

static e_errcode_t validate_ip_boundary(
    const char      *const p_ip_str
);

/*********************************************************************************************
 Private function implementations
 ********************************************************************************************/
/**
 * @brief Validate the boundary characters of an IPv4 string.
 *
 * This function performs lightweight checks before parsing:
 *  - The string must exist.
 *  - The string must not be empty.
 *  - The first and last characters must be decimal digits.
 *
 * Full IPv4 format validation is completed later while parsing each octet.
 *
 * @param[in] p_ip_str IPv4 string to be checked.
 *
 * @return e_errcode_t.
 *
 * @retval EC_RET_SUCCESS       Boundary checks passed.
 * @retval EC_RET_ARG_NULL      p_ip_str is NULL.
 * @retval EC_RET_INVALID_PARAM Invalid IPv4 string:
 *                              - IP string is empty.
 *                              - IP string starts with a non-digit character.
 *                              - IP string ends with a non-digit character.
 */
static e_errcode_t validate_ip_boundary(
    const char      *const p_ip_str
)
{
    e_errcode_t ret = EC_RET_SUCCESS;
    
    if (NULL == p_ip_str)
    {
        DBG_LOG("No IP string");
        ret = EC_RET_ARG_NULL;
    }
    else if ('\0' == p_ip_str[0])
    {
        DBG_LOG("Empty IP string");
        ret = EC_RET_INVALID_PARAM;
    }
    else if (IS_NOT_NUM(p_ip_str[0]))
    {
        DBG_LOG("'%c' starts the IP string", p_ip_str[0]);
        ret = EC_RET_INVALID_PARAM;
    }
    else
    {
        uint32_t last_idx = (uint32_t)strlen(p_ip_str) - 1U;

        if (IS_NOT_NUM(p_ip_str[last_idx]))
        {
            DBG_LOG("'%c' ends the IP string", p_ip_str[last_idx]);
            ret = EC_RET_INVALID_PARAM;
        }
    }

    return ret;
}

/**
 * @brief Consume the dot delimiter after the current IPv4 octet.
 *
 * For octet 0 to octet 2, a dot ('.') must follow the parsed octet.
 * On success, the caller's string pointer is advanced to the start of the
 * next octet.
 *
 * For the last octet, no delimiter is consumed.
 *
 * @param[in]     idx     Current octet index.
 *                        Valid range: 0 to (IPV4_OCTET_COUNT - 1).
 * @param[in,out] pp_str  Address of the current position in the IPv4 string.
 *                        On success, it may be advanced past the dot delimiter.
 *
 * @return e_errcode_t.
 *
 * @retval EC_RET_SUCCESS       Normal operation.
 * @retval EC_RET_ARG_NULL      pp_str is NULL.
 * @retval EC_RET_INVALID_PARAM Invalid parser state or IPv4 format:
 *                              - idx is out of range.
 *                              - *pp_str is NULL.
 *                              - A required octet is missing.
 *                              - An unexpected character appears after an octet.
 */
static e_errcode_t move_to_next_octet(
    const uint8_t   idx,
    const char      **pp_str            /* NOLINT(readability-identifier-naming) */
)
{
    e_errcode_t ret = EC_RET_SUCCESS;
    
    if (idx >= IPV4_OCTET_COUNT)
    {
        DBG_LOG("Invalid octet index %u", idx);
        ret = EC_RET_INVALID_PARAM;
    }
    else if (NULL == pp_str)
    {
        DBG_LOG("Address to the IP string is NULL");
        ret = EC_RET_ARG_NULL;
    }
    else if (NULL == *pp_str)
    {
        DBG_LOG("Invalid IP string");
        ret = EC_RET_INVALID_PARAM;
    }
    else
    {
        const char *p_str_cur = *pp_str;

        /**
         * For non-last octets, the parsed field must be followed by a dot delimiter.
         * If the string ends here, the next octet is missing. Any other character
         * means the current octet contains invalid trailing data.
         */
        if (idx < (IPV4_OCTET_COUNT - 1U))
        {
            if ('.' == *p_str_cur)
            {
                p_str_cur++;
            }
            else if ('\0' == *p_str_cur)
            {
                DBG_LOG("Missing octet %u", (idx + 2U));
                ret = EC_RET_INVALID_PARAM;
            }
            else
            {
                DBG_LOG("Invalid character '%c' in octet %u", *p_str_cur, (idx + 1U));
                ret = EC_RET_INVALID_PARAM;
            }
        }

        /* Return the updated position in IP string */
        if (EC_RET_SUCCESS == ret)
        {
            *pp_str = p_str_cur;
        }
    }

    return ret;
}

/**
 * @brief Parse the current dotted-decimal field into one IPv4 octet.
 *
 * Decimal digits are consumed from the current string position and accumulated
 * into a numeric octet value. Parsing stops at the first non-digit character,
 * which should be either a dot delimiter or the string terminator.
 *
 * On success, the caller's string pointer is advanced to the first character
 * after the parsed octet.
 *
 * @param[in,out] pp_str   Address of the current position in the IPv4 string.
 *                         On success, it is advanced past the parsed octet.
 * @param[out]    p_octet  Pointer to store the parsed octet value.
 *
 * @return e_errcode_t.
 *
 * @retval EC_RET_SUCCESS       Normal operation.
 * @retval EC_RET_ARG_NULL      Null pointer input:
 *                              - pp_str is NULL.
 *                              - p_octet is NULL.
 * @retval EC_RET_INVALID_PARAM Invalid IPv4 octet:
 *                              - *pp_str is NULL.
 *                              - Octet has no digit.
 *                              - Octet has more than 3 digits.
 *                              - Octet value is greater than 255.
 */
static e_errcode_t convert_octet(
    const char      **pp_str,           /* NOLINT(readability-identifier-naming) */
    uint32_t        *const p_octet
)
{
    e_errcode_t ret = EC_RET_SUCCESS;

    if ((NULL == pp_str) || (NULL == p_octet))
    {
        DBG_LOG("Parameter is NULL");
        ret = EC_RET_ARG_NULL;
    }
    else if (NULL == *pp_str)
    {
        DBG_LOG("Invalid IP string");
        ret = EC_RET_INVALID_PARAM;
    }
    else
    {
        const char *p_str_cur   = *pp_str;
        uint8_t     digit_count = 0U;

        /**
         * Parse one IPv4 octet in decimal form.
         *
         * The value is accumulated digit-by-digit:
         *  "192" -> ((1 * 10) + 9) * 10 + 2 = 192
         *
         * Validation is done during accumulation so that invalid octets are rejected
         * as soon as possible:
         *  - more than 3 digits
         *  - value greater than 255
         */
        while (IS_NUM(*p_str_cur))
        {
            *p_octet = ((*p_octet) * DECIMAL_BASE) + (uint32_t)(*p_str_cur - '0');

            digit_count++;

            if (IPV4_OCTET_DIGIT_MAX < digit_count)
            {
                DBG_LOG("Octet has more than %u digits", IPV4_OCTET_DIGIT_MAX);
                ret = EC_RET_INVALID_PARAM;
                break;
            }
            
            if (IPV4_OCTET_MAX < *p_octet)
            {
                DBG_LOG("Octet (%u) exceeds %u", *p_octet, IPV4_OCTET_MAX);
                ret = EC_RET_INVALID_PARAM;
                break;
            }

            p_str_cur++;
        }

        if (EC_RET_SUCCESS == ret)
        {
            /* Reject an empty field, for example the missing octet in "192..1.1". */
            if (0U == digit_count)
            {
                ret = EC_RET_INVALID_PARAM;
            }
            else
            {
                /* Return the updated parsing position to the caller. */
                *pp_str = p_str_cur;
            }
        }
    }

    return ret;
}

/*********************************************************************************************
 Public function implementations
 ********************************************************************************************/
e_errcode_t parse_ipv4(
    const char  *p_ip_str,
    uint32_t    *const p_ip_out
)
{
    uint32_t    ip  = 0U;
    e_errcode_t ret = EC_RET_SUCCESS;

    /**
     * Do not parse the input unless the caller provides valid output storage.
     * The input string is validated only after the output destination is safe to use.
     */
    if (NULL == p_ip_out)
    {
        DBG_LOG("p_ip_out is NULL");
        ret = EC_RET_ARG_NULL;
    }
    else
    {
        ret = validate_ip_boundary(p_ip_str);
    }

    /* Start parsing... */
    if (EC_RET_SUCCESS == ret)
    {
        /* Parse exactly four IPv4 octets from left to right. */
        for (uint8_t octet_idx = 0U; octet_idx < IPV4_OCTET_COUNT; octet_idx++)
        {
            uint32_t octet = 0U;

            /* Parse the current dotted-decimal field into one octet. */
            ret = convert_octet(&p_ip_str, &octet);

            if (EC_RET_SUCCESS == ret)
            {
                /**
                 * Build the 32-bit IPv4 value in network-byte order.
                 * After each non-last octet, consume the required dot delimiter before
                 * continuing with the next field.
                 */
                ip = ((ip << BIT_NUM_PER_BYTE) | octet);

                ret = move_to_next_octet(octet_idx, &p_ip_str);
                if (EC_RET_SUCCESS != ret)
                {
                    break;
                }
            }
            else    /**< convert_octet() failed */
            {
                /**
                 * convert_octet() failed before consuming any valid digit for this field.
                 * A dot or string terminator means the octet is empty.
                 */
                if (('\0' == *p_ip_str) || ('.' == *p_ip_str))
                {
                    DBG_LOG("Missing octet %u", (octet_idx + 1U));
                }
                else
                {
                    DBG_LOG("Fail to convert digits in octet %u", (octet_idx + 1U));
                }

                break;
            }
        }
    }

    if (EC_RET_SUCCESS == ret)
    {
        if ('\0' == *p_ip_str)
        {
            /* Complete the parse, return output to user */
            *p_ip_out = ip;
        }
        else
        {
            /**
             * Four octets were parsed successfully. Any remaining character means the
             * input contains extra fields or trailing data.
             *
             * Example:
             *  127.0.0.5.1
             *  192.168.2.70ab
             *  192.168.1.50.
             */
            DBG_LOG("The IP string is followed by some unwanted characters '%s'", p_ip_str);
            ret = EC_RET_INVALID_PARAM;
        }
    }

    return ret;
}

/*********************************************************************************************
 End of file
 ********************************************************************************************/
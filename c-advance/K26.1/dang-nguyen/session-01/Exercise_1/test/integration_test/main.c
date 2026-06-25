#include <stdio.h>
#include "ipv4_types.h"
#include "ipv4_parser.h"

#define APP_SUCCESS (0)
#define APP_FAILURE (-1)

#define ARG_NUM     (2)

int32_t main(int32_t argc, char *argv[])
{
    uint32_t ip_out = 0U;
    int32_t ret = APP_SUCCESS;

    if (ARG_NUM != argc)
    {
        printf("Usage: %s <IPv4 address>\n", argv[0]);
        printf("Example: %s 192.168.1.50\n", argv[0]);
        ret = APP_FAILURE;
    }
    else
    {
        e_errcode_t lib_ret = parse_ipv4(argv[1], &ip_out);
    
        if (EC_RET_SUCCESS != lib_ret)
        {
            printf("Invalid IPv4 address: %s\n", argv[1]);
            ret = APP_FAILURE;
        }
        else
        {
            printf("IPv4 address: %s\n", argv[1]);
            printf("Parsed value : 0x%08x (%u)\n", ip_out, ip_out);
        }
    }

    return ret;
}
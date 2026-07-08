/**
 * @file main.c
 * @brief Bitmask permission checker using enum flags and bitwise AND.
 * 
 * Demonstrates enum bit flags, has_permission() function with bitwise logic,
 * and discusses -fshort-enums ABI compatibility dangers.
 */

/*
 * Question:
 * Why might the size of this enum differ between a GCC compiler for an x86 PC 
 * vs an ARM Cortex-M micro-controller? What is the compiler flag -fshort-enums 
 * and why is it dangerous when linking libraries?
 *
 * Answer:
 * In C, the size of an enum is implementation-defined. This means the C
 * standard does not require every compiler or target architecture to use the
 * same enum size. A GCC compiler for an x86 PC commonly stores an enum using
 * the same size as int, which is typically 4 bytes. However, an embedded
 * compiler or a GCC build for an ARM Cortex-M target may use different ABI
 * rules or compiler options to reduce memory usage.
 *
 * The compiler flag -fshort-enums tells GCC to store an enum using the smallest 
 * integer type that can represent all enumerator values. For example, an enum 
 * only needs values from 0x00 to 0x0F, so with -fshort-enums the enum may become 
 * 1 byte instead of 4 bytes.
 *
 * This flag is dangerous when linking libraries because it changes the binary
 * interface of the code. If one object file or library is compiled with
 * -fshort-enums and another is compiled without it, both sides may disagree about 
 * the size and alignment of the same enum type. This can change struct layout, 
 * member offsets, padding, array element size, function argument passing, and 
 * return values.
 *
 * For example, if a struct contains an enum member, one library may expect the
 * enum field to occupy 4 bytes while another expects it to occupy only 1 byte.
 * The result can be corrupted data, invalid struct parsing, or hard-to-debug 
 * runtime bugs. Therefore, enum size should not be assumed in shared binary 
 * interfaces. If an exact size is required, use a fixed-width integer type such 
 * as uint8_t for the stored bitmask and use the enum only for named constants.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define EC_SUCCESS          ((int32_t)(0))
#define EC_FAILURE          ((int32_t)(1))

#define PERM_BIT_READ       (0U)
#define PERM_BIT_WRITE      (1U)
#define PERM_BIT_EXECUTE    (2U)
#define PERM_BIT_DELETE     (3U)

#define ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))

/**
 * @brief System permission bit flags.
 */
typedef enum e_sys_perms
{
    PERM_NONE    = 0U,
    PERM_READ    = (1U << PERM_BIT_READ),
    PERM_WRITE   = (1U << PERM_BIT_WRITE),
    PERM_EXECUTE = (1U << PERM_BIT_EXECUTE),
    PERM_DELETE  = (1U << PERM_BIT_DELETE),
    PERM_ALL     = (PERM_READ | PERM_WRITE | PERM_EXECUTE | PERM_DELETE)
} e_sys_perms_t;

/**
 * @brief Fixed-width permission mask storage type.
 */
typedef uint8_t perm_mask_t;

/**
 * @brief User permission information.
 */
typedef struct st_user_info
{
    const char  *p_perm_info;
    perm_mask_t perm_val;
} st_user_info_t;

/**
 * @brief Required permission check information.
 */
typedef struct st_perm_check
{
    const char  *p_message;
    perm_mask_t required_perm;
} st_perm_check_t;

/**
 * @brief Check whether a user has all required permissions.
 *
 * @param[in] user_perms     User permission bitmask.
 * @param[in] required_perms Required permission bitmask.
 * @return true if all required permissions are present, otherwise false.
 */
static bool has_permission(const perm_mask_t user_perms, const perm_mask_t required_perms);

/**
 * @brief Print one permission check result.
 *
 * @param[in] p_message      Permission check message.
 * @param[in] user_perms     User permission bitmask.
 * @param[in] required_perms Required permission bitmask.
 */
static void print_permission_check(const char *const p_message, const perm_mask_t user_perms, const perm_mask_t required_perms);

/**
 * @brief Print one user permission report.
 *
 * @param[in] user_id User ID for display.
 * @param[in] p_usr   User permission information.
 * @return EC_SUCCESS on success, otherwise EC_FAILURE.
 */
static int32_t print_user_permission_report(const size_t user_id, const st_user_info_t *const p_usr);

static const st_user_info_t g_user_info[] =
{
    /* p_perm_info */           /* perm_val */
    { "None",                   (perm_mask_t)PERM_NONE                                  },
    { "Read",                   (perm_mask_t)PERM_READ                                  },
    { "Write",                  (perm_mask_t)PERM_WRITE                                 },
    { "Execute",                (perm_mask_t)PERM_EXECUTE                               },
    { "Delete",                 (perm_mask_t)PERM_DELETE                                },
    { "(Read|Write)",           (perm_mask_t)(PERM_READ | PERM_WRITE)                   },
    { "(Read|Execute)",         (perm_mask_t)(PERM_READ | PERM_EXECUTE)                 },
    { "(Read|Delete)",          (perm_mask_t)(PERM_READ | PERM_DELETE)                  },
    { "(Read|Write|Execute)",   (perm_mask_t)(PERM_READ | PERM_WRITE | PERM_EXECUTE)    },
    { "(Read|Write|Delete)",    (perm_mask_t)(PERM_READ | PERM_WRITE | PERM_DELETE)     },
    { "(Read|Execute|Delete)",  (perm_mask_t)(PERM_READ | PERM_EXECUTE | PERM_DELETE)   },
    { "(Write|Execute)",        (perm_mask_t)(PERM_WRITE | PERM_EXECUTE)                },
    { "(Write|Delete)",         (perm_mask_t)(PERM_WRITE | PERM_DELETE)                 },
    { "(Write|Execute|Delete)", (perm_mask_t)(PERM_WRITE | PERM_EXECUTE | PERM_DELETE)  },
    { "(Execute|Delete)",       (perm_mask_t)(PERM_EXECUTE | PERM_DELETE)               },
    { "(All permissions)",      (perm_mask_t)PERM_ALL                                   }
};

static const st_perm_check_t g_perm_checks[] =
{
    /* p_message */                                         /* required_perm */
    { "Checking for Read permission...",                    (perm_mask_t)PERM_READ                                  },
    { "Checking for Write permission...",                   (perm_mask_t)PERM_WRITE                                 },
    { "Checking for Execute permission...",                 (perm_mask_t)PERM_EXECUTE                               },
    { "Checking for Delete permission...",                  (perm_mask_t)PERM_DELETE                                },
    { "Checking for Read AND Write...",                     (perm_mask_t)(PERM_READ | PERM_WRITE)                   },
    { "Checking for Read AND Execute...",                   (perm_mask_t)(PERM_READ | PERM_EXECUTE)                 },
    { "Checking for Read AND Delete...",                    (perm_mask_t)(PERM_READ | PERM_DELETE)                  },
    { "Checking for Read AND Write AND Execute...",         (perm_mask_t)(PERM_READ | PERM_WRITE | PERM_EXECUTE)    },
    { "Checking for Read AND Write AND Delete...",          (perm_mask_t)(PERM_READ | PERM_WRITE | PERM_DELETE)     },
    { "Checking for Read AND Execute AND Delete...",        (perm_mask_t)(PERM_READ | PERM_EXECUTE | PERM_DELETE)   },
    { "Checking for Write AND Execute...",                  (perm_mask_t)(PERM_WRITE | PERM_EXECUTE)                },
    { "Checking for Write AND Delete...",                   (perm_mask_t)(PERM_WRITE | PERM_DELETE)                 },
    { "Checking for Write AND Execute AND Delete...",       (perm_mask_t)(PERM_WRITE | PERM_EXECUTE | PERM_DELETE)  },
    { "Checking for Execute AND Delete...",                 (perm_mask_t)(PERM_EXECUTE | PERM_DELETE)               },
    { "Checking for All permissions...",                    (perm_mask_t)PERM_ALL                                   }
};

static bool has_permission(const perm_mask_t user_perms, const perm_mask_t required_perms)
{
    return ((user_perms & required_perms) == required_perms);
}

static void print_permission_check(const char *const p_message, const perm_mask_t user_perms, const perm_mask_t required_perms)
{
    printf("%s %s\n",
            p_message,
            (true == has_permission(user_perms, required_perms)) ? "GRANTED" : "DENIED");
}

static int32_t print_user_permission_report(const size_t user_id, const st_user_info_t *const p_usr)
{
    int32_t ret = EC_SUCCESS;

    if (NULL == p_usr)
    {
        printf("User info is NULL!\n");
        ret = EC_FAILURE;
    }
    else if ((NULL == p_usr->p_perm_info) || ('\0' == p_usr->p_perm_info[0]))
    {
        printf("Permission info is invalid!\n");
        ret = EC_FAILURE;
    }
    else
    {
        printf("User %zu %s: 0x%02X\n", user_id, p_usr->p_perm_info, p_usr->perm_val);

        for (size_t check_idx = 0U; check_idx < ARRAY_SIZE(g_perm_checks); check_idx++)
        {
            print_permission_check(g_perm_checks[check_idx].p_message, p_usr->perm_val, g_perm_checks[check_idx].required_perm);
        }

        printf("\n");
    }

    return ret;
}

int32_t main(void)
{
    int32_t ret = EC_SUCCESS;

    printf("=== Bitmask Permissions Tester ===\n");
    printf("Enum size: %zu bytes (Standard GCC)\n\n", sizeof(e_sys_perms_t));

    for (size_t user_idx = 0U; user_idx < ARRAY_SIZE(g_user_info); user_idx++)
    {
        ret = print_user_permission_report(user_idx + 1U, &g_user_info[user_idx]);

        if (EC_SUCCESS != ret)
        {
            break;
        }
    }

    return ret;
}
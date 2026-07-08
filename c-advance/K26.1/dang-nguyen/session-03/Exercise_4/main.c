#include <stdio.h>
#include <stdint.h>

#define EC_SUCCESS          ((int32_t)(0))
#define EC_FAILURE          ((int32_t)(1))

#define PACKED_ATTR         __attribute__((packed))

#define ARRAY_SIZE          (5U)

/**
 * @brief Unpacked union: uint32_t and uint8_t[5] array for padding demo.
 */
typedef union u_unpacked
{
    uint32_t val;
    uint8_t  arr[ARRAY_SIZE];
} u_unpacked_t;

/**
 * @brief Packed struct containing unpacked union - demonstrates tail padding.
 */
typedef struct PACKED_ATTR st_packed_with_unpacked_union
{
    u_unpacked_t unpacked_uni;
} st_packed_with_unpacked_union_t;

/**
 * @brief Packed union: uint32_t and uint8_t[5] array for packed demo.
 */
typedef union PACKED_ATTR u_packed
{
    uint32_t val;
    uint8_t  arr[ARRAY_SIZE];
} u_packed_t;

/**
 * @brief Packed struct containing packed union.
 */
typedef struct PACKED_ATTR st_packed_with_packed_union
{
    u_packed_t packed_uni;
} st_packed_with_packed_union_t;

/**
 * @brief Struct demonstrating hardware register.
 */
typedef struct st_hw_reg_bits
{
    uint32_t EN   : 1;
    uint32_t MODE : 3;
    uint32_t FLAG : 1;
    uint32_t res  : 27;
} st_hw_reg_bits_t;

/**
 * @brief Union containing hardware register.
 */
typedef union u_hw_reg
{
    uint32_t         ALL;
    st_hw_reg_bits_t BIT;
} u_hw_reg_t;

/**
 * @brief Demonstrate packed union, struct packing effects, and peripheral register pattern.
 * 
 * Tasks:
 * - Compare size of unpacked vs packed union in struct
 * - Implement struct bit-fields for hardware register access
 * - Use union to access register via ALL or BIT members
 * 
 * @return EC_SUCCESS on success, EC_FAILURE otherwise.
 */
int32_t main(void)
{
    int32_t ret = EC_SUCCESS;
    u_hw_reg_t reg = {0};

    printf("=== Advanced Nested Packing ===\n");

    /**
     * The unpacked union needs 5 bytes for its largest member, uint8_t arr[5].
     * However, assuming this target uses a 4-byte alignment requirement for this
     * union type, the compiler rounds the union size up from 5 bytes to 8 bytes
     * to satisfy that alignment requirement.
     *
     * Packing only the outer struct does not change the internal size or alignment
     * requirement of the nested unpacked union type. Therefore, the packed struct
     * that contains the unpacked union still has a size of 8 bytes.
     *
     * When the union itself is packed, its alignment requirement is reduced and
     * the compiler does not add tail padding. Therefore, the packed union size is
     * 5 bytes, and the packed struct containing it is also 5 bytes.
     */
    printf("Size of struct with UNPACKED union: %zu bytes\n", sizeof(st_packed_with_unpacked_union_t));
    printf("Size of struct with PACKED union: %zu bytes\n", sizeof(st_packed_with_packed_union_t));
    printf("\n");

    printf("=== Struct Bit-Fields & Hardware Mapping ===\n");
    printf("Size of hw_reg_bits_t: %zu bytes\n", sizeof(st_hw_reg_bits_t));

    printf("Register ALL before: 0x%08X\n", reg.ALL);
    printf("Setting EN bit via bit-field...\n");
    reg.BIT.EN = 1U;
    printf("Register ALL after: 0x%08X\n", reg.ALL);

    printf("Clearing register via ALL...\n");
    reg.ALL    = 0U;
    printf("Register ALL final: 0x%08X\n", reg.ALL);

    /**
     * Question:
     * Why does the strict CMSIS standard forbid using struct bit-fields for mapping 
     * hardware registers?
     * 
     * Answer:
     * Strict CMSIS forbids using C struct bit-fields to map hardware registers
     * because the C standard does not guarantee the exact bit layout of bit-fields
     * in memory. Hardware registers require exact bit positions. For example,
     * bit 0 must really mean bit 0 of the register. If a bit-field layout changes
     * between compilers, the firmware may write to the wrong hardware bit.
     *
     * Question:
     * What are the two compiler-dependent behaviors (endianness and padding boundaries) 
     * that break portability?
     * 
     * Answer:
     * 1. Endianness / bit allocation order:
     *    Different compilers or targets may allocate bit-fields from the least
     *    significant bit to the most significant bit, or in the opposite direction.
     *    Therefore, a field named EN may map to bit 0 on one compiler, but may map
     *    to a different bit position on another compiler.
     *
     * 2. Padding and storage-unit boundaries:
     *    The compiler decides how bit-fields are packed into storage units, when to
     *    insert padding bits, and when to start a new storage unit boundary. This can
     *    change the offset, size, and bit position of fields inside the struct.
     *
     * Because of these compiler-dependent behaviors, bit-fields are unsafe for
     * portable hardware register mapping. For hardware registers, use explicit
     * masks and shifts instead.
     */

    return ret;
}
/**
 * 1. What does hardware "Endianness" mean in terms of memory addresses?
 * 
 * Answer: Endianness describes how a CPU stores a multi-byte value in memory.
 * A multi-byte value occupies consecutive byte addresses.
 *
 * In a Big-Endian system, the most significant byte is stored at the lowest
 * memory address. For example, 0x12345678
 *
 *     Address:   base     base+1   base+2   base+3
 *     Byte:      0x12     0x34     0x56     0x78
 *
 * In a Little-Endian system, the least significant byte is stored at the
 * lowest memory address:
 *
 *     Address:   base     base+1   base+2   base+3
 *     Byte:      0x78     0x56     0x34     0x12
 *
 * 2. What do LSB (Least Significant Byte) and MSB (Most Significant Byte) mean in this context?
 * 
 * Answer: MSB means Most Significant Byte. It is the byte that carries the highest
 * place value in a multi-byte number. For 0x12345678, the MSB is 0x12.
 *
 * LSB means Least Significant Byte. It is the byte that carries the lowest
 * place value in a multi-byte number. For 0x12345678, the LSB is 0x78.
 *
 * 3. Why is it critically important for embedded engineers to understand endianness 
 * when transmitting data over a network or reading from a hardware sensor?
 * 
 * Answer: Because there might be differences in byte order between devices, 
 * embedded engineers must understand this concept to handle it correctly.
 *
 * For example, if a sensor sends the 16-bit value 0x1234 as two bytes 0x12 0x34, 
 * but the firmware interprets those bytes in the wrong order, the value becomes 0x3412. 
 * This can cause incorrect behaviours.
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define APP_SUCCESS (0)
#define APP_FAILURE (1)

#define UNION_SIZE  (4U)

#define TEST_TARGET             (0x11223344U)
#define LITTLE_ENDIAN_FIRST     (0x44U)
#define BIG_ENDIAN_FIRST        (0x11U)

typedef union u_endian_checker
{
    uint32_t full_word;
    uint8_t  bytes[UNION_SIZE];
} u_endian_checker_t;

int32_t main(void)
{
    u_endian_checker_t test = {0};
    uint8_t first_byte      = 0U;
    int32_t ret             = APP_SUCCESS;

    test.full_word = TEST_TARGET;
    first_byte = test.bytes[0];
    
    printf("=== Endianness Checker ===\n");
    printf("Stored Value: 0x%08" PRIX32 "\n", test.full_word);
    printf("First Byte in Memory: 0x%02" PRIX8 "\n", first_byte);

    if (LITTLE_ENDIAN_FIRST == first_byte)
    {
        printf("Result: This system is Little-Endian!\n");
    }
    else if (BIG_ENDIAN_FIRST == first_byte)
    {
        printf("Result: This system is Big-Endian!\n");
    }
    else
    {
        printf("Result: Unknown byte order!\n");
        ret = APP_FAILURE;
    }

    return ret;
}
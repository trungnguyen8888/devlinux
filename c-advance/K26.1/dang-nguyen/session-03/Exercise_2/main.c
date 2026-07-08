/**
 * @file main.c
 * @brief Analyze structure padding, optimized member order, and packed structs.
 *
 * A packed structure removes padding and places members at consecutive byte
 * offsets. This saves memory, but it may place members at unaligned addresses.
 *
 * Direct access to a packed member is usually handled by the compiler because
 * the compiler knows the object is packed. However, taking the address of a
 * packed member and storing it in a normal pointer is unsafe:
 *
 *     int *p_i = &packed_instance.i;
 *     *p_i = 20;
 *
 * The pointer type int * normally assumes that the pointed address satisfies
 * int alignment. In a packed struct, the int member may be stored at an
 * unaligned address. Dereferencing such a pointer can cause inefficient memory
 * access, undefined behavior, or a hardware fault on architectures that do not 
 * support unaligned access, such as some ARM configurations. In production 
 * embedded code, avoid taking pointers to packed members. Use direct member 
 * access, byte-wise parsing, or memcpy-style copying into a properly aligned 
 * local variable instead.
 */
#include <stdio.h>
#include <stddef.h> // For offsetof()

#define EC_SUCCESS                  (0)
#define EC_FAILURE                  (-1)

#define PACKED_DIRECT_TEST_VALUE    (10)
#define PACKED_POINTER_TEST_VALUE   (20)

#define PACKED_ATTR                 __attribute__((packed))

/**
 * @brief Original unoptimized structure.
 */
typedef struct st_unoptimized {
    char   c;      // 1 byte
    int    i;      // 4 bytes
    double d;      // 8 bytes
    short  s;      // 2 bytes
} st_unoptimized_t;

/**
 * @brief Optimized structure ordered from largest member to smallest member.
 */
typedef struct st_optimized
{
    double d;
    int    i;
    short  s;
    char   c;
} st_optimized_t;

/**
 * @brief Packed version of the original structure.
 */
typedef struct PACKED_ATTR st_packed
{
    char   c;
    int    i;
    double d;
    short  s;
} st_packed_t;

/**
 * @brief Entry point for stack depth monitor demonstration.
 * 
 * Initializes stack base address and calls recurse_with_monitor() to demonstrate
 * stack consumption tracking and overflow detection.
 * 
 * @return 0 on success, -1 if stack limit was exceeded.
 */
int main(void)
{
    st_packed_t my_st = {0};
    int ret = EC_SUCCESS;

    printf("=== Struct Padding Analyzer ===\n");
    printf("[Unoptimized Struct]\n");
    printf("Size: %zu bytes\n", sizeof(st_unoptimized_t));
    printf("Offsets: c(%zu), i(%zu), d(%zu), s(%zu)\n\n",
            offsetof(st_unoptimized_t, c),
            offsetof(st_unoptimized_t, i),
            offsetof(st_unoptimized_t, d),
            offsetof(st_unoptimized_t, s));
    
    printf("[Optimized Struct]\n");
    printf("Size: %zu bytes\n", sizeof(st_optimized_t));
    printf("Offsets: d(%zu), i(%zu), s(%zu), c(%zu)\n\n",
            offsetof(st_optimized_t, d),
            offsetof(st_optimized_t, i),
            offsetof(st_optimized_t, s),
            offsetof(st_optimized_t, c));
    
    printf("[Packed Struct]\n");
    printf("Size: %zu bytes\n", sizeof(st_packed_t));
    printf("Offsets: c(%zu), i(%zu), d(%zu), s(%zu)\n\n",
            offsetof(st_packed_t, c),
            offsetof(st_packed_t, i),
            offsetof(st_packed_t, d),
            offsetof(st_packed_t, s));
    
    my_st.i = PACKED_DIRECT_TEST_VALUE;
    printf("Attempting direct access to packed member... Success! Value = %d\n", my_st.i);

#ifdef ENABLE_UNSAFE_POINTER_DEMO
    {
        int *p_i = &my_st.i;

        printf("Attempting pointer access to packed member... ");
        *p_i = PACKED_POINTER_TEST_VALUE;
        printf("Value = %d\n", *p_i);
    }
#endif

    return ret;
}
#ifndef UTILITY__UTILITY_H
#define UTILITY__UTILITY_H

/**
 * Various utility macros and functions.
 */

#include <stdbool.h> /* bool */
#include <stdio.h> /* stderr, fprintf() */
#include <string.h> /* memset(), memcpy(), memmove() */
#include <wchar.h> /* wchar_t */

#include "utility/xalloc.h" /* xcalloc(), xreallocarray(), xmemdup() */

/* If the compiler does not have __has_builtin, always make it 0. */
#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

/* success indicator value */
#define OK 0

/* indicate integer error value */
#define ERROR 1

/* Abort the program after printing an error message. */
#define ABORT(message) do { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, (message)); \
    abort(); \
} while (0)

/* Wrap these around statements when an if branch is involved to hint to the
 * compiler whether an if statement is likely (or unlikely) to be true.
 * For example:
 *   if (UNLIKELY(pointer == NULL)) {
 *       printf("I am unlikely to occur\n");
 *   }
 * These should ONLY be used when it is guaranteed that a branch is executed
 * only very rarely.
 */
#if __has_builtin(__builtin_expect)
#   define LIKELY(x) __builtin_expect(!!(x), 1)
#   define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#   define LIKELY(x) (x)
#   define UNLIKELY(x) (x)
#endif

/* Use this to hint to the compiler that specific code is unreachable. */
#if __has_builtin(__builtin_unreachable)
#   define UNREACHABLE __builtin_unreachable()
#else
#   define UNREACHABLE ABORT("unreachable")
#endif

/* Assert that statement @x is true. If this is not the case, the program is
 * aborted.
 *
 * Only use this for really critical parts where the rest of the code will not
 * function if a specific error occurs.  E.g. a memory allocation error.
 */
#define ASSERT(x, message) do { \
    if (UNLIKELY(!(x))) { \
        ABORT(message); \
    } \
} while (0)

/* Get the maximum number of digits this number type could take up.
 *
 * UINT8_MAX  255 -> 3
 * UINT16_MAX 65535 -> 5
 * UINT32_MAX 4294967295 -> 10
 * UINT64_MAX 18446744073709551615 -> 20
 *
 * The default case INT_MIN is chosen so that static array allocations fail.
 */
#define MAXIMUM_DIGITS(number_type) ( \
        sizeof(number_type) == 1 ? 3 : \
        sizeof(number_type) == 2 ? 5 : \
        sizeof(number_type) == 4 ? 10 : \
        sizeof(number_type) == 8 ? 20 : INT_MIN)

/* Get the size of a statically sized array. */
#define SIZE(a) (sizeof(a) / sizeof(*(a)))

/* Turn the argument into a string. */
#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

/* Allocate a block of memory and put it into @p. */
#define ALLOCATE(p, n) ((p) = xreallocarray(NULL, (n), sizeof(*(p))))

/* Allocate a zeroed out block of memory and put it into @p. */
#define ALLOCATE_ZERO(p, n) ((p) = xcalloc((n), sizeof(*(p))))

/* Zero out a memory block. */
#define ZERO(p, n) (memset((p), 0, sizeof(*(p)) * (n)))

/* Resize allocated array to given number of elements.
 *
 * Example usage:
 * ```
 * int *integers;
 *
 * ALLOCATE(integers, 60);
 * integers[59] = 64;
 *
 * REALLOCATE(integers, 120);
 * integers[119] = 8449;
 * ```
 */
#define REALLOCATE(p, n) ((p) = xreallocarray((p), (n), sizeof(*(p))))

/* Copy a memory block. */
#define COPY(dest, src, n) \
    (memcpy((dest), (src), sizeof(*(dest)) * (n)))

/* Move a memory block.
 *
 * This should be used if the memory blocks intersect.
 */
#define MOVE(dest, src, n) \
    (memmove((dest), (src), sizeof(*(dest)) * (n)))

/* Duplicate a memory block. */
#define DUPLICATE(p, n) (xmemdup((p), sizeof(*(p)) * (n)))

/* Sort an array with given sort compare function. */
#define SORT(array, n, compare) \
    qsort(array, n, sizeof(*(array)), compare)

/* Get the maximum of two numbers. */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Get the minimum of two numbers. */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Get the absolute difference between two numbers. */
#define ABSOLUTE_DIFFERENCE(a, b) ((a) < (b) ? (b) - (a) : (a) - (b))

/* Run a program in the background using given arguments.
 *
 * @argv is a list of strings terminated by NULL.
 * @argv[0] is the name of the program.
 *
 * @return 0 if the child process started, 1 otherwise.
 */
int run_program(char *const *argv);

/* Run @command within a shell in the background.
 *
 * @return 0 if the child process started, 1 otherwise.
 */
int run_shell(const char *command);

/* Run @command as command within a shell and get the first line from it.
 *
 * Up to 1024 bytes are read.
 *
 * @return NULL if an error occured.
 */
char *run_shell_and_get_output(const char *command);

/* Check if a character is a line ending character.
 *
 * This includes \n, \v, \f and \r.
 */
int islineend(int character);

/* Get the graphical width of a wide character. */
int wcwidth(wchar_t wide_character);

/* Get the length of @string up to a maximum of @max_length. */
size_t strnlen(const char *string, size_t max_length);

/* Compare two strings while ignoring case.
 *
 * @return 0 for equality, a negative value if @string1 < @string2,
 *         otherwise a positive value.
 */
int strcasecmp(const char *string1, const char *string2);

/* Match a string against a pattern.
 *
 * Pattern metacharacters are ?, *, [.  They can be escaped using \ to match
 * them literally.  All other \. sequences are matched literally (with the \).
 *
 * An opening bracket [ without a matching close ] is matched literally.
 *
 * @pattern is a shell-style glob pattern, e.g. "*.[ch]".
 *          It may be NULL, then the empty string is used.
 * @string is the string to match against.
 *         It may be NULL, then the empty string is used.
 *
 * @return if the string matches the pattern.
 */
bool matches_pattern(_Nullable const char *pattern,
        _Nullable const char *string);

#endif

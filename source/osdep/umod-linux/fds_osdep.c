#define _GNU_SOURCE
#include <stdarg.h>
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <fds_assert.h>
#include <shared/fds_types.h>

#define STACK_TRACE_SIZE             (32)

/*
 * fds_panic
 * ---------
 */

static void __attribute__((noinline))
            __attribute__((noreturn))
fds_panic_abort(char *panic_string)
{
    void     *stack[STACK_TRACE_SIZE];
    char    **symb;
    size_t    i, size;

    size = backtrace(stack, STACK_TRACE_SIZE);
    symb = backtrace_symbols(stack, size);

    /* TODO(Sean):
     * In this case, where does printf() go for non-standalone tests?
     * Is the stdout stream redirected or is it print to the screen?
     * The panic message and stack trace should be logged in a log file.
     *
     * TODO(Sean):
     * Second thing....   It's possible to get symbol names using
     * gcc option "-rdynamic" when compiling, but it has some performance
     * implications.  Could be useful in debug build tests.
     */
    printf("Dump recent %ld stack frames\n", size);
    for (i = 0; i < size; i++) {
        printf("%s\n", symb[i]);
    }

    /* abort() should be the last system call in this function
     */
    abort();
}

void
fds_panic(const char *fmt, ...)
{
    va_list args;
    char panic_str_buf[2048];
    int ret;

    va_start(args, fmt);
    ret = vsnprintf(panic_str_buf, sizeof(panic_str_buf), fmt, args);
    /* If bytes written is greater than equal, then the string was truncated.
     * If truncated, then add null terminating character to the last offset
     * in the panic string buffer.
     */
    if (ret >= sizeof(panic_str_buf)) {
        panic_str_buf[sizeof(panic_str_buf) - 1] = '\0';
    }
    va_end(args);

    /* This will show up on the stack, and when using GDB, panic_str_buf
     * should show up fully formatted.
     */
    fds_panic_abort(panic_str_buf);
}

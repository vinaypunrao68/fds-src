#define _GNU_SOURCE
#include <stdarg.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <fds_assert.h>

#define STACK_TRACE_SIZE             (32)

/*
 * fds_panic
 * ---------
 */
void
fds_panic(const char *fmt, ...)
{
    va_list   ap;
    size_t    i, size;
    void     *stack[STACK_TRACE_SIZE];
    char    **symb;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    size = backtrace(stack, STACK_TRACE_SIZE);
    symb = backtrace_symbols(stack, size);

    printf("Dump recent %ld stack frames\n", size);
    for (i = 0; i < size; i++) {
        printf("%s\n", symb[i]);
    }
    abort();
}

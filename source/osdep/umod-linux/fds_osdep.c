/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

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
#include <syslog.h>
#include <stdlib.h>

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
    char symStr[16384];
    int symStrPos = 0;
    int written = 0;
    size_t symStrSize = sizeof(symStr);

    size = backtrace(stack, STACK_TRACE_SIZE);
    symb = backtrace_symbols(stack, size);

    /*
     * TODO(Sean):First thing....   It's possible to get symbol names using
     *            gcc option "-rdynamic" when compiling, but it has some performance
     *            implications.  Could be useful in debug build tests.
     *
     *            Second thing. This is not c++ code, and not in fds namespace.
     *            Why???
     */
    for (i = 0; i < size; i++) {
        written += snprintf(symStr + symStrPos,
                            symStrSize - written,
                            "%s",
                            symb[i]);
        symStrPos += written;
        /* if the position is beyond the buffer, then terminate the loop
         * and null terminate the string.
         */
        if (symStrPos >= symStrSize) {
            symStr[symStrSize - 1] = '\0';
            break;
        }
    }

    /* Output to both log file and syslog */
    /* TODO(Sean):  This file is not compiled as c++ and in fds namespace.
     *              Not going to try to fix it now.  just dump the panic
     *              string to syslog for now.
     */
    // LOGERROR << "Panic'ing ... " << panic_string < " -- " << symStr;
    syslog(LOG_ERR, "FDS_PROC Panic'ing... %s -- %s", panic_string, symStr);
    free(symb);

    /* abort() should be the last system call in this function
     */
    abort();
}

void
fds_panic(const char *fmt, ...)
{
    va_list args;
    char panic_str_buf[4096];
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

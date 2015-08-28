/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <cstdarg>
#include <cstdio>
#include <fds_assert.h>
#include <cstdlib>
#include <ostream>
#include <iostream>
#include <unistd.h>
#include <sys/param.h>

/*
 * fds_panic
 * ---------
 */

static void __attribute__((noinline))
            __attribute__((noreturn))
fds_panic_abort(char *panic_string)
{
    std::cerr << panic_string << std::endl;;
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
    if (ret >= static_cast<int>(sizeof(panic_str_buf))) {
        panic_str_buf[sizeof(panic_str_buf) - 1] = '\0';
    }
    va_end(args);

    /* This will show up on the stack, and when using GDB, panic_str_buf
     * should show up fully formatted.
     */
    fds_panic_abort(panic_str_buf);
}

ssize_t
fds_exe_path(char* pBuf, size_t len)
{
    char szTmp[32];

    snprintf(szTmp, 32, "/proc/%d/exe", getpid());
    ssize_t bytes = MIN(readlink(szTmp, pBuf, len), static_cast<ssize_t >(len - 1));

    if (bytes >= 0)
        pBuf[bytes] = '\0';

    return bytes;
}

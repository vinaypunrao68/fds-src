/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <unistd.h>

#include <fds_assert.h>

int stack_size = 128;

void
panic_test()
{
    if (--stack_size == 0) {
        fds_panic("panic test");
    } else {
        panic_test();
    }
}

int
main(int argc, char *argv[])
{
    panic_test();
}

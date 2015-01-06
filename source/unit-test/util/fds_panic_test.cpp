#include <unistd.h>
#include <fds_assert.h>

/*
 * This is a simple test to demonstrate that an unformatted string,
 * when calling fds_panic(), will show up on the GDB backtrace as a
 * formatted string.
 *
 * NOTE(Sean):
 * This is not an interesting test, and should not be
 * integrated with an automatic testing infrastructure, since only
 * thing it does it a core dump.
 */
void __attribute__((noinline))
wtf()
{
    char *tmp_str = "formed panic string";
    fds_panic("%s:%d => %s\n",
              __func__, __LINE__, tmp_str);
}

void __attribute__((noinline))
bar()
{
    wtf();
}

void __attribute__((noinline))
foo()
{
bar();
}


int
main(int argc, char*argv[])
{
    foo();
}


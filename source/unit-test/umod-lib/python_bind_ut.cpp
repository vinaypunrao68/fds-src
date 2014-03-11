/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds_python_bind.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sched.h>
#include <iostream>

#include <fds_assert.h>

using namespace std;

namespace fds {

}  // namespace fds


int
python_bind_test(void)
{
    int             res = 0;
    fds::PythonBind      p_bind("python_bind_ut");

    res = p_bind.load_module();
    fds_assert(res == 0);

    res = p_bind.call("python_bind_echo", 1, "hello world");
    fds_assert(res == 0);

    return (res);
}

int
main(int argc, char* argv[])
{
    int             res = 0;

    fds::PythonBind::py_initialize();
    res = python_bind_test();
    fds::PythonBind::py_finalize();

    return (res);
}

/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <Am.h>

int main(int argc, char* argv[]) {
    std::unique_ptr<fds::AmProcess> process(new fds::AmProcess(argc, argv, false));
    process->main();
    return 0;
}

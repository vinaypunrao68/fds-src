/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <Om.h>

int main(int argc, char* argv[]) {
    std::unique_ptr<fds::Om> process(new fds::Om(argc, argv, false));
    process->main();
    return 0;
}

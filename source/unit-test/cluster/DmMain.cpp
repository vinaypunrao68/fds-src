/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <Dm.h>

int main(int argc, char* argv[]) {
    std::unique_ptr<fds::DmProcess> process(new fds::DmProcess(argc, argv, false));
    process->main();
    return 0;
}

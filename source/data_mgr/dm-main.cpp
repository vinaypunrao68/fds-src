/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <DataMgr.h>

namespace fds {
DataMgr *dataMgr;
}  // namespace fds

int main(int argc, char *argv[])
{
    fds::Module *dmVec[] = {
        &gl_DmPlatform,
        NULL
    };
    fds::dataMgr = new fds::DataMgr(argc, argv, &gl_DmPlatform, dmVec);
    fds::dataMgr->setup();
    fds::dataMgr->run();
    delete fds::dataMgr;
    return 0;
}

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
    int ret = fds::dataMgr->main();
    delete fds::dataMgr;
    return ret;
}

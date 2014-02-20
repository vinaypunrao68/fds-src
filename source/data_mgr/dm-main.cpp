/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <DataMgr.h>

namespace fds {
DataMgr *dataMgr;
}  // namespace fds

int main(int argc, char *argv[])
{
    fds::dataMgr = new fds::DataMgr(argc, argv, "dm.conf", "fds.dm.");
    fds::Module *dmVec[] = {
        fds::dataMgr,
        nullptr
    };
    fds::dataMgr->setup(argc, argv, dmVec);
    fds::dataMgr->run();
    delete fds::dataMgr;
    return 0;
}

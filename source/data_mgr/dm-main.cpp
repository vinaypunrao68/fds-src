/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <DataMgr.h>
#include <net/net-service.h>

namespace fds {
DataMgr *dataMgr;
}  // namespace fds

int gdb_stop = 0;

int main(int argc, char *argv[])
{
    fds::Module *dmVec[] = {
        &gl_DmPlatform,
        &gl_NetService,
        NULL
    };
    // while (gdb_stop == 0) {
    //    sleep(1);
    // }
    fds::dataMgr = new fds::DataMgr(argc, argv, &gl_DmPlatform, dmVec);
    int ret = fds::dataMgr->main();
    delete fds::dataMgr;
    return ret;
}

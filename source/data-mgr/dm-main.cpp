/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <DataMgr.h>
#include <net/net-service.h>
#include <util/fds_stat.h>

#include "platform/platform_process.h"

namespace fds {
// TODO(Rao): Get rid of this singleton
DataMgr *dataMgr;
}  // namespace fds

int gdb_stop = 0;

class DMMain : public PlatformProcess
{
 public:
    DMMain(int argc, char *argv[]) {
        /* Construct all the modules */
        dm = new fds::DataMgr(this);
        // TODO(Rao): Get rid of this singleton
        dataMgr = dm;

        /* Create the dependency vector */
        static fds::Module *dmVec[] = {
            &fds::gl_fds_stat,
            &gl_DmPlatform,
            &gl_NetService,
            dm,
            NULL
        };

        /* Init platform process */
        init(argc, argv, "fds.dm.", "dm.log", &gl_DmPlatform, dmVec);

        /* Daemonize */
        fds_bool_t noDaemon = get_fds_config()->get<bool>("fds.dm.testing.test_mode", false);
        if (noDaemon == false) {
            daemonize();
        }
    }

    virtual ~DMMain() {
        /* Destruct created module objects */
        delete dm;
    }

    virtual int run() {
        return dm->run();
    }

    fds::DataMgr* dm;
};

int main(int argc, char *argv[])
{
    auto dmMain = new DMMain(argc, argv);
    auto ret = dmMain->main();
    delete dmMain;
    return ret;
}

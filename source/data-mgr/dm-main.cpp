/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <DataMgr.h>

#include <net/SvcProcess.h>

#include <fdsp/DMSvc.h>
#include <DMSvcHandler.h>

namespace fds {
// TODO(Rao): Get rid of this singleton
DataMgr *dataMgr;
}  // namespace fds

int gdb_stop = 0;

class DMMain : public SvcProcess
{
 public:
    DMMain(int argc, char *argv[]) {
        /* Construct all the modules */
        dm = new fds::DataMgr(this);
        // TODO(Rao): Get rid of this singleton
        dataMgr = dm;

        /* Create the dependency vector */
        static fds::Module *dmVec[] = {
            dm,
            NULL
        };

        /* Init Service process */
        init<fds::DMSvcHandler, fpi::DMSvcProcessor>(argc, argv, "platform.conf",
                "fds.dm.", "dm.log", dmVec);
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
    /* Based on command line arg --foreground is set, don't daemonize the process */
    fds::FdsProcess::checkAndDaemonize(argc, argv);

    auto dmMain = new DMMain(argc, argv);
    auto ret = dmMain->main();
    delete dmMain;
    return ret;
}

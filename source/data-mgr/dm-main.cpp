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

        /* Before calling init, close all file descriptors.  Later, we may daemonize the
         * process, in which case we may be closing all existing file descriptors while
         * threads may access the file descriptor.
         */
        closeAllFDs();

        /* Init Service process */
        init<fds::DMSvcHandler, fpi::DMSvcProcessor>(argc, argv, "platform.conf",
                "fds.dm.", "dm.log", dmVec);

        /* Daemonize */
        fds_bool_t noDaemon = get_fds_config()->get<bool>("fds.dm.testing.test_mode", false);
        // xxx: Daemonize should be the first thing that needs to happen
        if (false && noDaemon == false) {
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

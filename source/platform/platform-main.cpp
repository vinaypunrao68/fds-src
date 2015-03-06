/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <string>
#include <iostream>  // NOLINT
#include <fds_process.h>

#include <fdsp/svc_types_types.h>
#include <net/SvcProcess.h>
#include <platform/platformmanager.h>
#include <platform/svchandler.h>

namespace fds {

class PlatformMain : public SvcProcess {
  public:
    PlatformMain(int argc, char *argv[]) {
        platform = new fds::pm::PlatformManager();
        /* Create the dependency vector */
        static fds::Module *modules[] = {
            platform,
            NULL
        };

        closeAllFDs();

        init<fds::pm::SvcHandler, fpi::PlatNetSvcProcessor>(argc, argv, "platform.conf",
                                                            "fds.pm.", "pm.log", modules);
        /* Daemonize */
        fds_bool_t noDaemon = get_fds_config()->get<bool>("fds.pm.testing.test_mode", false);
        if (noDaemon == false) {
            daemonize();
        }
    }

    virtual int run() {
        return platform->run();
    }
  protected:
    fds::pm::PlatformManager* platform;

};
} // namespace fds

int main(int argc, char **argv) {
    auto pmMain = new fds::PlatformMain(argc, argv);
    auto ret = pmMain->main();
    delete pmMain;
    return ret;
}

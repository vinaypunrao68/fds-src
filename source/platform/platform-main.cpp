/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <string>
#include <iostream>  // NOLINT
#include <fds_process.h>

#include <fdsp/fds_service_types.h>
#include <net/SvcProcess.h>
#include <platform/platformmanager.h>
#include <platform/svchandler.h>
#include <disk_plat_module.h>
namespace fds {

class PlatformMain : public SvcProcess {
  public:
    PlatformMain(int argc, char *argv[]) {
        platform = new fds::pm::PlatformManager();
        /* Create the dependency vector */
        static fds::Module *modules[] = {
            platform,
            &gl_DiskPlatMod,
            NULL
        };

        closeAllFDs();

        init<fds::pm::SvcHandler, fpi::PlatNetSvcProcessor>(argc, argv, "platform.conf",
                                                            "fds.pm.", "pm.log", modules);

        util::Properties& props = platform->getProperties();
        props.setData(&svcInfo_.props);

        /* Daemonize */
        fds_bool_t noDaemon = get_fds_config()->get<bool>("fds.pm.testing.test_mode", false);
        if (noDaemon == false) {
            daemonize();
        }
    }

    virtual void setupSvcInfo_() {
        SvcProcess::setupSvcInfo_();
        const fpi::NodeInfo& nodeInfo = platform->getNodeInfo();
        svcInfo_.svc_id.svc_uuid.svc_uuid = nodeInfo.uuid;
        platform->loadProperties();
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

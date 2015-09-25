/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <string>
#include <iostream>  // NOLINT

#include "fds_process.h"
#include "net/SvcProcess.h"
#include "net/SvcMgr.h"

#include "disk_plat_module.h"
#include "platform/platform_manager.h"
#include "platform/svc_handler.h"

namespace fds
{
    class PlatformMain : public SvcProcess
    {
        public:
            PlatformMain (int argc, char *argv[])
            {
                platform = new fds::pm::PlatformManager();

                auto handler = boost::make_shared <fds::pm::SvcHandler> (this, platform);
                auto processor = boost::make_shared <fpi::PlatNetSvcProcessor> (handler);
                init (argc, argv, "platform.conf", "fds.pm.", "pm.log", nullptr, handler, processor);

                gl_DiskPlatMod.mod_startup();
                bool fDumpDiskMap = get_fds_config()->get<fds_bool_t>("fds.pm.dump_diskmap",false);

                if (fDumpDiskMap) {
                    GLOGWARN << "exiting as fds.pm.dump_diskmap is true";
                    exit(0);
                }

                platform->updateServiceInfoProperties(&svcInfo_.props);
            }

            void setupSvcInfo_()
            {
                gl_DiskPlatMod.mod_init (nullptr);
                platform->mod_init (nullptr);
                SvcProcess::setupSvcInfo_();

                const fpi::NodeInfo& nodeInfo = platform->getNodeInfo();
                svcInfo_.svc_id.svc_uuid.svc_uuid = nodeInfo.uuid;
                LOGNOTIFY << "Svc info overrriden to: " << fds::logString(svcInfo_);
            }

            int run()
            {
                platform->run();

                return 0;
            }

        protected:
            fds::pm::PlatformManager* platform;
    };

} // namespace fds

int main (int argc, char *argv[])
{
    /* Based on command line arg --foreground is set, don't daemonize the process */
    fds::FdsProcess::checkAndDaemonize (argc, argv);

    auto pmMain = new fds::PlatformMain (argc, argv);
    auto ret = pmMain->main();
    delete pmMain;
    return ret;
}

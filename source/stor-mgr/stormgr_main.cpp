/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <StorMgr.h>
#include <policy_tier.h>
#include <sm-platform.h>
#include <net/net-service.h>

#include "platform/platform_process.h"

class SMMain : public PlatformProcess
{
 public:
    SMMain(int argc, char *argv[]) {
        /* Construct all the modules */
        sm = new fds::ObjectStorMgr(this);
        // TODO(Rao): Get rid of this singleton
        objStorMgr  = sm;

        /* Create the dependency vector */
        static fds::Module *smVec[] = {
            &diskio::gl_dataIOMod,
            &fds::gl_SmPlatform,
            &fds::gl_NetService,
            &fds::gl_tierPolicy,
            sm,
            nullptr
        };

        /* Init platform process */
        init(argc, argv, "fds.sm.", "sm.log", &gl_SmPlatform, smVec);

        /* Daemonize */
        fds_bool_t noDaemon = get_fds_config()->get<bool>("fds.sm.testing.test_mode", false);
        if (noDaemon == false) {
            daemonize();
        }
    }

    virtual ~SMMain() {
        /* Destruct created module objects */
        // TODO(brian): Revisit this; currently sm will be deleting itself
        // when this gets called... the following line will cause a double delete
        // delete sm;
        LOGDEBUG << "ENDING SM PROCESS";
    }

    virtual int run() {
        return sm->run();
    }

    fds::ObjectStorMgr* sm;
};

int main(int argc, char *argv[])
{
    auto smMain = new SMMain(argc, argv);
    auto ret = smMain->main();
    delete smMain;
    return ret;
}


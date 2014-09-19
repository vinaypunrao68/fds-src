/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 * Replace XX with your adapter name.
 */
#include <unistd.h>
#include <string>
#include <iostream>
#include <adapter.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <platform/platform-lib.h>
#include <sm-platform.h>
#include <policy_tier.h>
#include <StorMgr.h>

namespace fds {

class SMProc : public FdsProbeProcess
{
  public:
    SMProc(int argc, char *argv[])
    {
        sm = new ObjectStorMgr(this);
        objStorMgr = sm;

        static Module *smVec[] = {
            /* Same as SM. */
            &diskio::gl_dataIOMod,
            &gl_SmPlatform,
            &gl_NetService,
            &gl_tierPolicy,
            &gl_objStats,
            sm,

            /* Probe modules. */
            &gl_fds_stat,
            &gl_probeS3Eng,
            &gl_SMProbeMod,
            NULL
        };
        init(argc, argv, "fds.sm.", "sm-probe.log", &gl_SmPlatform, smVec);
        bool no_daemon = get_fds_config()->get<bool>("fds.sm.testing.test_mode", false);

        svc_probe = &gl_SMProbeMod;
        if (no_daemon == false) {
            daemonize();
        }
    }
    virtual ~SMProc() {
        delete sm;
    }
    int run() override {
        return sm->run();
    }

  protected:
    ObjectStorMgr  *sm;
};

}  // namespace fds

int main(int argc, char **argv)
{
    fds::SMProc probe(argc, argv);
    return probe.main();
}

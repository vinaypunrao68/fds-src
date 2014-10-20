/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <string>
#include <iostream>
#include <probe-adapter.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <platform/platform-lib.h>
#include <am-nbd.h>
#include <AccessMgr.h>
#include <net-platform.h>

namespace fds {

class TestProcess : public ProbeProcess
{
  public:
    TestProcess(int argc, char *argv[],
                const std::string &log, ProbeMod *probe, Module **vec) :
        ProbeProcess(argc, argv, log, probe, vec, "fds.am.") {}

    void proc_pre_startup() override
    {
        ProbeProcess::proc_pre_startup();
        am = AccessMgr::unique_ptr(new AccessMgr("AMMain Probe", this));
    }
    int run() override
    {
        while (1) {
            sleep(2);
        }
        am->run();
        return 0;
    }

  private:
    AccessMgr::unique_ptr am;
};

}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *probe_vec[] = {
        &fds::gl_NbdBlockMod,
        NULL
    };
    fds::TestProcess probe(argc, argv, "am-probe.log", &fds::gl_AmProbeMod, probe_vec);
    return probe.main();
}

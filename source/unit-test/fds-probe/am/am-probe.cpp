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
#include <probe-adapter.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <platform/platform-lib.h>
#include <am-nbd.h>

namespace fds {

class TestProcess : public ProbeProcess
{
  public:
    TestProcess(int argc, char *argv[],
                const std::string &log, ProbeMod *probe, Module **vec) :
        ProbeProcess(argc, argv, log, probe, vec) {}

    int run() override
    {
        while (1) {
            sleep(2);
        }
        return 0;
    }
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

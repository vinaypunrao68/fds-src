/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <pm_probe.h>
#include <fds_module.h>
#include <fds_process.h>
#include <persistent-layer/dm_io.h>
#include <iostream>

namespace fds {

class BlkProbe : public FdsProcess
{
  public:
    virtual ~BlkProbe() {}
    BlkProbe(int argc, char **argv, Module **vec)
        : FdsProcess(argc, argv, "platform.conf", "fds.plat.", "probe-blk.log", vec) {}

    void proc_pre_startup() override {}
    void proc_pre_service() override {}
    int run()
    {
        gl_probeBlkLib.probe_run_main(&gl_PM_ProbeMod);
        return 0;
    }
};

}  // namespace fds

int
main(int argc, char **argv)
{
    fds::Module *pm_probe_vec[] = {
        &diskio::gl_dataIOMod,
        &fds::gl_probeBlkLib,
        &fds::gl_PM_ProbeMod,
        nullptr
    };
    fds::BlkProbe probe(argc, argv, pm_probe_vec);
    return probe.main();
}

/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <fdsp/fds_service_types.h>
#include <net/SvcProcess.h>
#include <fdsp/PlatNetSvc.h>
#include <fdsp/OMSvc.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>

namespace fds {
struct TestSvcProcess : SvcProcess {
    TestSvcProcess(int argc, char *argv[])
    : SvcProcess(argc, argv, "platform.conf", "fds.dm.", "dm.log", nullptr) {
    }

    virtual int run() override {
        LOGNOTIFY << "Doing work";
        while (true) {
            sleep(1000);
        }
        return 0;
    }
};
}  // namespace fds

int main(int argc, char *argv[])
{
    std::unique_ptr<fds::TestSvcProcess> process(new fds::TestSvcProcess(argc, argv));
    process->main();
    // int a = 0;
    // a++;
    return 0;
}

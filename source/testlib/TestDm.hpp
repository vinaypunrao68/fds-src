#ifndef TESTDM_HPP_
#define TESTDM_HPP_

/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <fdsp/PlatNetSvc.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>
#include <net/PlatNetSvcHandler.h>
#include <DMSvcHandler.h>
#include "fdsp/common_constants.h"
#include <fdsp/DMSvc.h>
#include <DataMgr.h>

namespace fds {


struct TestDm : SvcProcess {
    TestDm(int argc, char *argv[], bool initAsModule);
    virtual int run() override;
    DataMgr* getDataMgr() { return dm.get(); }

    std::unique_ptr<DataMgr> dm;
    fds::Module *dmVec[2];
};

TestDm::TestDm(int argc, char *argv[], bool initAsModule)
{
    dm.reset(new DataMgr(this));
    dmVec[0] = dm.get();
    dmVec[1] = nullptr;

    auto handler = boost::make_shared<DMSvcHandler>(this, *dm);
    auto processor = boost::make_shared<fpi::DMSvcProcessor>(handler);

    init(argc, argv, initAsModule, "platform.conf",
         "fds.dm.", "dm.log", dmVec, handler, processor, fpi::commonConstants().DM_SERVICE_NAME);

}

int TestDm::run() {
    LOGNORMAL << "Doing work";
    readyWaiter.done();
    shutdownGate_.waitUntilOpened();
    return 0;
}


}  // namespace fds

#endif    // TESTDM_HPP_

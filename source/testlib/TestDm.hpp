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

    /**
     * Note on Thrift service compatibility:
     * Because asynchronous service requests are routed manually, any new
     * PlatNetSvc version MUST extend a previous PlatNetSvc version.
     * Only ONE version of PlatNetSvc API can be included in the list of
     * multiplexed services.
     *
     * For other new major service API versions (not PlatNetSvc), pass
     * additional pairs of processor and Thrift service name.
     */
    TProcessorMap processors;
    processors.insert(std::make_pair<std::string,
        boost::shared_ptr<apache::thrift::TProcessor>>(
            fpi::commonConstants().DM_SERVICE_NAME, processor));

    init(argc, argv, initAsModule, "platform.conf",
         "fds.dm.", "dm.log", dmVec, handler, processors);

}

int TestDm::run() {
    LOGNORMAL << "Doing work";
    readyWaiter.done();
    shutdownGate_.waitUntilOpened();
    return 0;
}


}  // namespace fds

#endif    // TESTDM_HPP_

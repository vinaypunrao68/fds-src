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
     *
     * For service that extends PlatNetSvc, add the processor twice using
     * Thrift service name as the key and again using 'PlatNetSvc' as the
     * key. Only ONE major API version is supported for PlatNetSvc.
     *
     * All other services:
     * Add Thrift service name and a processor for each major API version
     * supported.
     */
    TProcessorMap processors;

    /**
     * When using a multiplexed server, handles requests from DMSvcClient.
     */
    processors.insert(std::make_pair<std::string,
        boost::shared_ptr<apache::thrift::TProcessor>>(
            fpi::commonConstants().DM_SERVICE_NAME, processor));

    /**
     * It is common for SvcLayer to route asynchronous requests using an
     * instance of PlatNetSvcClient. When using a multiplexed server, the
     * processor map must have a key for PlatNetSvc.
     */
    processors.insert(std::make_pair<std::string,
        boost::shared_ptr<apache::thrift::TProcessor>>(
            fpi::commonConstants().PLATNET_SERVICE_NAME, processor));

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

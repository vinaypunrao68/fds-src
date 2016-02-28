/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/make_shared.hpp>
#include <fds_module_provider.h>
#include <net/net_utils.h>
#include <net/SvcRequestPool.h>
#include <net/SvcServer.h>
#include <net/SvcMgr.h>
#include <fdsp_utils.h>
#include <fiu-control.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include <util/fiu_util.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/profiler.h>

#include "fdsp/common_constants.h"

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

struct SvcMgrModuleProvider : CommonModuleProviderIf {
    SvcMgrModuleProvider(const std::string &configFile, fds_uint64_t uuid, int port) {
        /* config */
        std::string configBasePath = "fds.dm.";
        boost::shared_ptr<FdsConfig> config(new FdsConfig(configFile, 0, {nullptr}));
        configHelper_.init(config, configBasePath);
        config->set("fds.dm.svc.uuid", uuid);
        config->set("fds.dm.svc.port", port);

        /* timer */
        timer_.reset(new FdsTimer());

        /* counters */
        cntrsMgr_.reset(new FdsCountersMgr(configBasePath));

        /* threadpool */
        threadpool_.reset(new fds_threadpool(3));

        /* service mgr */
        auto handler = boost::make_shared<PlatNetSvcHandler>(this);
        auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
        fpi::SvcInfo svcInfo;
        svcInfo.svc_id.svc_uuid.svc_uuid = 
                static_cast<int64_t>(configHelper_.get<fds_uint64_t>("svc.uuid"));
        svcInfo.ip = net::get_local_ip(configHelper_.get_abs<std::string>("fds.nic_if","lo"));
        svcInfo.svc_port = configHelper_.get<int>("svc.port");
        svcInfo.incarnationNo = util::getTimeStampSeconds();
        svcMgr_.reset(new SvcMgr(this, handler, processor, svcInfo, fpi::commonConstants().PLATNET_SERVICE_NAME));
        svcMgr_->mod_init(nullptr);
    }

    virtual FdsConfigAccessor get_conf_helper() const override {
        return configHelper_;
    }

    virtual boost::shared_ptr<FdsConfig> get_fds_config() const override {
        return configHelper_.get_fds_config();
    }

    virtual FdsTimerPtr getTimer() const override { return timer_; }

    virtual boost::shared_ptr<FdsCountersMgr> get_cntrs_mgr() const override { return cntrsMgr_; }

    virtual fds_threadpool *proc_thrpool() const override { return threadpool_.get(); }

    virtual SvcMgr* getSvcMgr() override { return svcMgr_.get(); }

 protected:
    FdsConfigAccessor configHelper_;
    FdsTimerPtr timer_;
    boost::shared_ptr<FdsCountersMgr> cntrsMgr_;
    fds_threadpoolPtr threadpool_;
    boost::shared_ptr<SvcMgr> svcMgr_;
};
using SvcMgrModuleProviderPtr = boost::shared_ptr<SvcMgrModuleProvider>;


TEST(SvcServer, stop)
{
    int port = 10000;
    auto handler = boost::make_shared<PlatNetSvcHandler>(nullptr);
    auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);

    auto server = boost::make_shared<SvcServer>(port, processor,
        fpi::commonConstants().PLATNET_SERVICE_NAME, nullptr);
    server->start();

    sleep(1);

    /* Stop the server.  This will block until stop() completes */
    server->stop();
}

/**
* @brief Test for SvcServer stop
*/
TEST(SvcServer, multi_stop)
{
    int port = 10000;
    auto handler = boost::make_shared<PlatNetSvcHandler>(nullptr);
    auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);

    for (int i = 0; i < 10; i++) {
        /* Create and start the server */
        auto server = boost::make_shared<SvcServer>(port, processor,
            fpi::commonConstants().PLATNET_SERVICE_NAME, nullptr);
        server->start();

        sleep(1);

        /* Create two clients */
        boost::shared_ptr<FdsConfig> pEmpty;
        auto client1 = allocRpcClient<fpi::PlatNetSvcClient>("127.0.0.1", port,
            SvcMgr::MAX_CONN_RETRIES, "", pEmpty);
        auto client2 = allocRpcClient<fpi::PlatNetSvcClient>("127.0.0.1", port,
            SvcMgr::MAX_CONN_RETRIES, "", pEmpty);

        /* Stop the server.  This will block until stop() completes */
        server->stop();

        std::cout << "Iteration: " << i << " completed";
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

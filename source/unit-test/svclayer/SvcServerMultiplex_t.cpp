/*
 * Copyright 2016 Formation Data Systems, Inc.
 *
 * Integration test.
 *
 * Runs a multiplexed SvcServer. The Thrift service used is OMSvc, which 
 * extends PlatNetSvc. Checks usage of multiplexed clients against the
 * multiplexed SvcServer.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/make_shared.hpp>
#include <fds_module_provider.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/profiler.h>

#include "fdsp/common_constants.h"
#include "net/fdssocket.h"
#include "fds_process.h"
#include "om-svc-handler.h"
#include "orchMgr.h"
#include "orch-mgr/om-service.h"

// Get template using fake data store; don't do this outside of test fixtures
#include "orch-mgr/om-common/om-svc-handler.cpp"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TMultiplexedProtocol.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace FDS_ProtocolInterface {

/**
 * @details
 *
 */
class FakeOMSvcClient : public OMSvcClient {

private:
    FakeOMSvcClient() = delete;

public:
    /**
     * If multiplexed, construct using a TMultiplexedProtocol.
     */
    explicit FakeOMSvcClient(boost::shared_ptr<::apache::thrift::protocol::TProtocol> p) : OMSvcClient(p) {}
};

} // namespace FDS_ProtocolInterface

namespace fds {

extern OrchMgr *orchMgr;
OM_Module *omModule;

OM_Module *OM_Module::om_singleton()
{
    return omModule;
}

/**
 * @brief A fake data store
 * @details
 * Inherits from no other, but must define interfaces used by OmSvcHandler<DataStoreT>.
 */
class FakeDataStore {

public:
    kvstore::ConfigDB::ReturnType getLocalDomain(fds_ldomid_t id, LocalDomain& localDomain);
    bool getSvcMap(std::vector<fpi::SvcInfo>& records);
    bool setCapacityUsedNode(const int64_t svcUuid, const unsigned long usedCapacityInBytes) {
        return true;
    }
    fpi::ServiceStatus getStateSvcMap(const int64_t svcUuid) {
        return fpi::SVC_STATUS_INVALID;
    }
};

/**
 * @details
 * Same as OmSvcHandler, except overrides 'getStatus' which is used
 * in this integration test. This test actually runs a server and
 * these overrides are used to handle requests.
 */
template<class DataStoreT>
class FakeOmSvcHandler : public OmSvcHandler<DataStoreT> {

public:
    explicit FakeOmSvcHandler(CommonModuleProviderIf *provider);
    virtual ~FakeOmSvcHandler() {}

    virtual fpi::ServiceStatus getStatus(const int32_t ignore) {
        return fpi::SVC_STATUS_STARTED;
    }

    virtual fpi::ServiceStatus getStatus(boost::shared_ptr<int32_t>& ignore) {
        return fpi::SVC_STATUS_STARTED;
    }

protected:
    fpi::OMSvcClientPtr createOMSvcClient(const std::string& strIPAddress,
        const int32_t& port) override;
};

template <class DataStoreT>
FakeOmSvcHandler<DataStoreT>::FakeOmSvcHandler(CommonModuleProviderIf *provider)
: OmSvcHandler<DataStoreT>(provider) {
}

template <class DataStoreT>
fpi::OMSvcClientPtr FakeOmSvcHandler<DataStoreT>::createOMSvcClient(
    const std::string& strIPAddress,
    const int32_t& port) {

    // not used in this integration test
    return nullptr;
}

/**
 * An integration test generally requires instantation of an FdsProcess.
 *
 * The process has responsibility for initializing the global logger.
 * Most FDS code depends on the global logger.
 * The process IS-A module, so this also makes libConfig data available
 * to test code via the global fds::g_fdsprocess pointer.
 *
 * A nice property of process initialization is that command line arguments
 * such as --fds.feature_toggle.common.enable_foo=true are copied into
 * the libConfig data.
 */
class FakeOMProcess : public FdsProcess {
  public:
    FakeOMProcess(int argc, char * argv[], const std::string & config,
                 const std::string & basePath, Module * vec[]) {
        init(argc, argv, config, basePath, "svcserver_multiplex_gtest.log", vec);
    }

    virtual int run() override {
        return 0;
    }
};

} // namespace fds

/**
 * A test fixture used to initialize and tear-down expensive
 * shared resources. In this case, a multiplexed server.
 */
class SvcServerTest : public ::testing::Test {

protected:

    // An expensive shared resource
    static boost::shared_ptr<fds::SvcServer> pServer_;

    static fds::FakeDataStore* pDataStore_;

    static void SetUpTestCase()
    {
        int port = 10000;

        auto pDataStore = new fds::FakeDataStore();
        auto pHandler = boost::make_shared<fds::FakeOmSvcHandler<fds::FakeDataStore>>(nullptr);
        pHandler->setConfigDB(pDataStore);

        // Set up a multiplexed server using one processer with two keys
        auto processor = boost::make_shared<fpi::OMSvcProcessor>(pHandler);
        fds::TProcessorMap processors;
        processors.insert(std::make_pair<std::string,
            boost::shared_ptr<apache::thrift::TProcessor>>(
                fpi::commonConstants().OM_SERVICE_NAME, processor));
        processors.insert(std::make_pair<std::string,
            boost::shared_ptr<apache::thrift::TProcessor>>(
                fpi::commonConstants().PLATNET_SERVICE_NAME, processor));

        pServer_ = boost::make_shared<fds::SvcServer>(port, processors, fds::g_fdsprocess);
        pServer_->start();
        sleep(1);
    }

    static void TearDownTestCase()
    {
        if (pServer_) {
            pServer_->stop();
        }
        pServer_.reset();
        delete pDataStore_;
    }
};

fds::FakeDataStore* SvcServerTest::pDataStore_ = nullptr;
boost::shared_ptr<fds::SvcServer> SvcServerTest::pServer_ = nullptr;

TEST_F(SvcServerTest, MultiplexClients)
{
    int port = 10000;
    // This is an integration test because of dependency on the network
    auto pSocket    = boost::make_shared<fds::net::Socket>("127.0.0.1", port);
    auto pTransport = boost::make_shared<::apache::thrift::transport::TFramedTransport>(pSocket);
    auto pProtocol  = boost::make_shared<::apache::thrift::protocol::TBinaryProtocol>(pTransport);

    // Validates client using key 'PlatNetSvc' for the multiplexed client
    auto pMultiProtocol = 
        boost::make_shared<::apache::thrift::protocol::TMultiplexedProtocol>(pProtocol,
            fpi::commonConstants().PLATNET_SERVICE_NAME);
    boost::shared_ptr<FDS_ProtocolInterface::FakeOMSvcClient> pClient1 =
        boost::make_shared<FDS_ProtocolInterface::FakeOMSvcClient>(pMultiProtocol);

    bool isConnected = pSocket->connect(20);

    EXPECT_EQ(isConnected, true);

    auto ignore = boost::make_shared<int32_t>(0);
    auto foo1 = pClient1->getStatus(ignore);

    // If the RPC worked, here is the expected status...
    EXPECT_EQ(foo1, fpi::SVC_STATUS_STARTED);

    // Try the same RPC, but using key 'OMSvc' for the multiplexed client

    auto pMultiProtocol2 =
        boost::make_shared< ::apache::thrift::protocol::TMultiplexedProtocol>(pProtocol,
            fpi::commonConstants().OM_SERVICE_NAME);
    boost::shared_ptr<FDS_ProtocolInterface::FakeOMSvcClient> pClient2 =
        boost::make_shared<FDS_ProtocolInterface::FakeOMSvcClient>(pMultiProtocol2);

    auto foo2 = pClient2->getStatus(0);

    // If the RPC worked, here is the expected status...
    EXPECT_EQ(foo2, fpi::SVC_STATUS_STARTED);
}

TEST_F(SvcServerTest, MultiplexFeatureToggle)
{
    // Guarantees that feature toggle is set
    bool enableMultiplexedServices = false;
    if (fds::g_fdsprocess) {
        // The module provider might not supply the base path we want,
        // so make our own config access. This is libConfig data, not
        // to be confused with FDS configuration (platform.conf).
        fds::FdsConfigAccessor configAccess(fds::g_fdsprocess->get_fds_config(),
            "fds.feature_toggle.");
        enableMultiplexedServices = configAccess.get<bool>("common.enable_multiplexed_services", false);
    }
    EXPECT_EQ(enableMultiplexedServices, true);
}

TEST_F(SvcServerTest, InvalidService)
{
    int port = 10000;
    // This is an integration test because of dependency on the network
    auto pSocket    = boost::make_shared<fds::net::Socket>("127.0.0.1", port);
    auto pTransport = boost::make_shared<::apache::thrift::transport::TFramedTransport>(pSocket);
    auto pProtocol  = boost::make_shared<::apache::thrift::protocol::TBinaryProtocol>(pTransport);

    // Uses known bad key for the multiplexed client
    auto pMultiProtocol = 
        boost::make_shared<::apache::thrift::protocol::TMultiplexedProtocol>(pProtocol,
            "KNOWN_BAD_KEY");
    boost::shared_ptr<FDS_ProtocolInterface::FakeOMSvcClient> pClient =
        boost::make_shared<FDS_ProtocolInterface::FakeOMSvcClient>(pMultiProtocol);

    bool isConnected = pSocket->connect(20);

    EXPECT_EQ(isConnected, true);

    // Should throw with description:
    // "TMultiplexedProcessor: Unknown service: KNOWN_BAD_KEY"
    EXPECT_ANY_THROW(pClient->getStatus(0));
}

TEST_F(SvcServerTest, SimplexClient)
{
    int port = 10000;
    // This is an integration test because of dependency on the network
    auto pSocket    = boost::make_shared<fds::net::Socket>("127.0.0.1", port);
    auto pTransport = boost::make_shared<::apache::thrift::transport::TFramedTransport>(pSocket);
    auto pProtocol  = boost::make_shared<::apache::thrift::protocol::TBinaryProtocol>(pTransport);

    // simplex client, which currently will not work against
    // multiplexed server
    boost::shared_ptr<FDS_ProtocolInterface::FakeOMSvcClient> pClient =
        boost::make_shared<FDS_ProtocolInterface::FakeOMSvcClient>(pProtocol);

    bool isConnected = pSocket->connect(20);

    EXPECT_EQ(isConnected, true);

    // Throws with description:
    // "No more data to read"
    EXPECT_ANY_THROW(pClient->getStatus(0));
}

int main(int argc, char** argv) {

    // Overrides libConfig data with command line key/value pairs
    // Initializes globals, including the global FDS process pointer
    fds::FakeOMProcess fakeProcess(argc, argv, "platform.conf", "fds.om.", NULL);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


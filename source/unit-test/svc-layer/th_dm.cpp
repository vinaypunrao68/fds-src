/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <chrono>
#include <atomic>
#include <thread>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include "TestAMSvc.h"
#include "TestDMSvc.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/profiler.h>

using ::testing::AtLeast;
using ::testing::Return;

using namespace fds;  // NOLINT
using namespace apache::thrift; // NOLINT
using namespace apache::thrift::protocol; // NOLINT
using namespace apache::thrift::transport; // NOLINT
using namespace apache::thrift::server; // NOLINT
using namespace apache::thrift::concurrency; // NOLINT

struct DMTest : BaseTestFixture
{
    DMTest();
    void serve();
 protected:
    boost::shared_ptr<TThreadedServer> server_;
};

class TestDMSvcHandler : virtual public FDS_ProtocolInterface::TestDMSvcIf {
 public:
    explicit TestDMSvcHandler(DMTest* dmTest) {
        dmTest_ = dmTest;
    }    /* Connect to DM */
    int32_t associate(const std::string& ip, const int32_t port) {
        return 0;
    }
    void updateCatalog(const ::FDS_ProtocolInterface::AsyncHdr& asyncHdr,
                   const ::FDS_ProtocolInterface::UpdateCatalogOnceMsg& payload)
    {
    }

    int32_t associate(boost::shared_ptr<std::string>& ip,
                      boost::shared_ptr<int32_t>& port) {
        int cntr = connCntr_++;
        boost::shared_ptr<fpi::TestAMSvcClient> amClient;
        boost::shared_ptr<TTransport> amSock(new TSocket(*ip, *port));
        boost::shared_ptr<TFramedTransport> amTrans(new TFramedTransport(amSock));
        boost::shared_ptr<TProtocol> amProto(new TBinaryProtocol(amTrans));
        amClient.reset(new fpi::TestAMSvcClient(amProto));
        amTrans->open();
        clientTbl_[cntr] = amClient;

        std::cout << "associated with " << ip << ":" << port << " id: " << cntr << std::endl;
        return cntr;
    }

    void updateCatalog(boost::shared_ptr<::FDS_ProtocolInterface::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<::FDS_ProtocolInterface::UpdateCatalogOnceMsg>& payload)
    {
        std::cout << "Received put object. Sending response " << std::endl;
        // TODO(Rao): Swap the src and dst id
        clientTbl_[asyncHdr->msg_src_id]->\
            updateCatalogRsp(*asyncHdr, ::FDS_ProtocolInterface::UpdateCatalogOnceRspMsg());
    }

 protected:
    std::atomic<int> connCntr_;
    std::unordered_map<int, boost::shared_ptr<fpi::TestAMSvcClient>> clientTbl_;
    DMTest *dmTest_;
};

DMTest::DMTest()
{
    /* setup server */
    int dmPort = this->getArg<int>("dm-port");
    boost::shared_ptr<TestDMSvcHandler> handler(new TestDMSvcHandler(this));
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    boost::shared_ptr<TProcessor> processor(
        new ::FDS_ProtocolInterface::TestDMSvcProcessor(handler));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(dmPort));
    boost::shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());
    server_.reset(new TThreadedServer(processor, serverTransport,
                                      transportFactory, protocolFactory));
    std::cout << "Server init at port: " << dmPort << std::endl;
}

void DMTest::serve()
{
    server_->serve();
}


TEST_F(DMTest, test)
{
    server_->serve();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("dm-port", po::value<int>()->default_value(9097), "dm port");
    DMTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

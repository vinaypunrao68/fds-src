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
// #include <thrift/server/TThreadPoolServer.h>
// #include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include "TestAMSvcSync.h"

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

struct AMServerTest : BaseTestFixture
{
    AMServerTest();
    void serve();
 protected:
    boost::shared_ptr<TNonblockingServer> server_;
};

class TestAMSvcSyncHandler : virtual public FDS_ProtocolInterface::TestAMSvcSyncIf {
 public:
    explicit TestAMSvcSyncHandler(AMServerTest* amTest) {
        amTest_ = amTest;
    }

    /* Connect to AM */
    void getBlob(std::string& _return, const std::string& domainName, const std::string& volumeName,
        const std::string& blobName, const int32_t length, const int32_t offset) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void getBlob(std::string& _return, boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName,
                    boost::shared_ptr<int32_t>& length,
                    boost::shared_ptr<int32_t>& offset) {
        // Your implementation goes here
        printf("getBlob\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        _return = "Fuck you XDI";
    }

 protected:
    // std::atomic<int> connCntr_;
    // std::unordered_map<int, boost::shared_ptr<fpi::TestAMSvcClient>> clientTbl_;
    AMServerTest *amTest_;
};

AMServerTest::AMServerTest()
{
    /* setup server */
    int amPort = this->getArg<int>("am-port");
    boost::shared_ptr<TestAMSvcSyncHandler> handler(new TestAMSvcSyncHandler(this));
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    boost::shared_ptr<TProcessor> processor(
        new ::FDS_ProtocolInterface::TestAMSvcSyncProcessor(handler));
    // boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(amPort));
    // boost::shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());
    boost::shared_ptr<ThreadManager> threadManager =
            ThreadManager::newSimpleThreadManager(30);
    boost::shared_ptr<PosixThreadFactory> threadFactory(new PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

    server_.reset(new TNonblockingServer(
        processor, protocolFactory, amPort, threadManager));
    std::cout << "Server init at port: " << amPort << std::endl;
}

void AMServerTest::serve()
{
    server_->serve();
}


TEST_F(AMServerTest, test)
{
    server_->serve();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("am-port", po::value<int>()->default_value(9092), "am port");
    AMServerTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

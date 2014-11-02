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
#include <thrift/server/TNonblockingServer.h>
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include "TestAMSvc.h"
#include "TestSMSvc.h"

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

struct SMTest : BaseTestFixture
{
    SMTest();
    void serve();
 protected:
    boost::shared_ptr<TServer> server_;
};

class TestSMSvcHandler : virtual public FDS_ProtocolInterface::TestSMSvcIf {
 public:
    explicit TestSMSvcHandler(SMTest* smTest) {
        smTest_ = smTest;
        connCntr_ = 0;
    }    /* Connect to SM */
    int32_t associate(const std::string& ip, const int32_t port) {
        return 0;
    }
    void putObject(const  ::FDS_ProtocolInterface::AsyncHdr& asyncHdr,
                      const  ::FDS_ProtocolInterface::PutObjectMsg& payload) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
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

    void putObject(boost::shared_ptr< ::FDS_ProtocolInterface::AsyncHdr>& asyncHdr,
                      boost::shared_ptr< ::FDS_ProtocolInterface::PutObjectMsg>& payload)
    {
        asyncHdr->rqRcvdTs = asyncHdr->rqHndlrTs = asyncHdr->rspSerStartTs =
            asyncHdr->rspSendStartTs = util::getTimeStampNanos();
        std::cout << "Received put object. Sending response " << std::endl;
        // TODO(Rao): Swap the src and dst id
        clientTbl_[asyncHdr->msg_src_uuid.svc_uuid]->\
            putObjectRsp(*asyncHdr, ::FDS_ProtocolInterface::PutObjectRspMsg());
    }

 protected:
    std::atomic<int> connCntr_;
    std::unordered_map<int, boost::shared_ptr<fpi::TestAMSvcClient>> clientTbl_;
    SMTest *smTest_;
};

SMTest::SMTest()
{
    /* setup server */
    int smPort = this->getArg<int>("sm-port");
    boost::shared_ptr<TestSMSvcHandler> handler(new TestSMSvcHandler(this));
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    boost::shared_ptr<TProcessor> processor(
        new ::FDS_ProtocolInterface::TestSMSvcProcessor(handler));
    if (this->getArg<std::string>("server") == "threaded") {
        boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(smPort));
        boost::shared_ptr<TTransportFactory> transportFactory;
        if (this->getArg<std::string>("transport") == "framed") {
            transportFactory.reset(new TFramedTransportFactory());
        } else {
            transportFactory.reset(new TBufferedTransportFactory());
        }
        server_.reset(new TThreadedServer(processor, serverTransport,
                                          transportFactory, protocolFactory));
        std::cout << "Threaded Server init at port: " << smPort << std::endl;
    } else {
        boost::shared_ptr<ThreadManager> threadManager =
            ThreadManager::newSimpleThreadManager(5);
        boost::shared_ptr<PosixThreadFactory> threadFactory =
            boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
        threadManager->threadFactory(threadFactory);
        threadManager->start();
        server_.reset(new TNonblockingServer(processor,
                                             protocolFactory, smPort, threadManager));
        std::cout << "Nonblocking Server init at port: " << smPort << std::endl;
    }
}

void SMTest::serve()
{
    server_->serve();
}


TEST_F(SMTest, test)
{
    server_->serve();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("server", po::value<std::string>()->default_value("threaded"), "Server type")
        ("transport", po::value<std::string>()->default_value("framed"), "transport type")
        ("sm-port", po::value<int>()->default_value(9092), "sm port");
    SMTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

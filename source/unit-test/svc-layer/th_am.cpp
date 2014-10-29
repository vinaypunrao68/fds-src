/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <chrono>
#include <utility>
#include <vector>
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

struct AMTest : BaseTestFixture
{
    AMTest();
    virtual ~AMTest();

    void serve();

    std::atomic<uint32_t> putsIssued_;
    std::atomic<uint32_t> putsSuccessCnt_;
    std::atomic<uint32_t> putsFailedCnt_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;

    std::thread *t_;
    boost::shared_ptr<TThreadedServer> server_;
};

struct TestAMServer {
    TestAMServer();
};

class TestAMSvcHandler : virtual public FDS_ProtocolInterface::TestAMSvcIf {
 public:
    explicit TestAMSvcHandler(AMTest* amTest) {
        amTest_ = amTest;
    }

    int32_t associate(const std::string& myip, const int32_t port)
    {
        return 0;
    }

    int32_t associate(boost::shared_ptr<std::string>& myip,
                      boost::shared_ptr<int32_t>& port)
    {
        return 0;
    }

    void putObjectRsp(const  ::FDS_ProtocolInterface::AsyncHdr& asyncHdr,
                      const  ::FDS_ProtocolInterface::PutObjectRspMsg& payload) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }


    void putObjectRsp(boost::shared_ptr< ::FDS_ProtocolInterface::AsyncHdr>& asyncHdr,
                      boost::shared_ptr< ::FDS_ProtocolInterface::PutObjectRspMsg>& payload)
    {
        if (asyncHdr->msg_code != ERR_OK) {
            amTest_->putsFailedCnt_++;
        } else {
            amTest_->putsSuccessCnt_++;
        }
        if (amTest_->putsIssued_ == (amTest_->putsSuccessCnt_ + amTest_->putsFailedCnt_)) {
            amTest_->endTs_ = util::getTimeStampNanos();
        }
    }
    void updateCatalogRsp(const  ::FDS_ProtocolInterface::AsyncHdr& asyncHdr,
                          const  ::FDS_ProtocolInterface::UpdateCatalogOnceRspMsg& payload) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void updateCatalogRsp(boost::shared_ptr< ::FDS_ProtocolInterface::AsyncHdr>& asyncHdr,
                          boost::shared_ptr< ::FDS_ProtocolInterface::UpdateCatalogOnceRspMsg>& payload)  // NOLINT
    {
    }

 protected:
    AMTest *amTest_;
};


AMTest::AMTest()
{
    putsIssued_ = 0;
    putsSuccessCnt_ = 0;
    putsFailedCnt_ = 0;

    int amPort = this->getArg<int>("am-port");
    boost::shared_ptr<TestAMSvcHandler> handler(new TestAMSvcHandler(this));
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    boost::shared_ptr<TProcessor> processor(
        new ::FDS_ProtocolInterface::TestAMSvcProcessor(handler));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(amPort));
    boost::shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());
    server_.reset(new TThreadedServer(processor, serverTransport,
                                     transportFactory, protocolFactory));
    std::cout << "Server init at port: " << amPort << std::endl;
}

void AMTest::serve()
{
    t_ = new std::thread(std::bind(&TThreadedServer::serve, server_.get()));
}

AMTest::~AMTest()
{
    server_->stop();
    t_->join();
}

TEST_F(AMTest, test)
{
    int dataCacheSz = 100;
    int connCnt = this->getArg<int>("conn-cnt");
    int nPuts =  this->getArg<int>("puts-cnt");
    std::string smIp = this->getArg<std::string>("sm-ip");
    std::string myIp = this->getArg<std::string>("my-ip");
    int smPort = this->getArg<int>("sm-port");
    int amPort = this->getArg<int>("am-port");

    int volId = 0;
    std::vector<std::pair<int, boost::shared_ptr<fpi::TestSMSvcClient>>> smClients(connCnt);

    /* start server for responses */
    serve();

    /* Wait a bit */
    sleep(2);

    /* Create connections against SM */
    for (int i = 0; i < connCnt; i++) {
        boost::shared_ptr<TTransport> smSock(new TSocket(smIp, smPort));
        boost::shared_ptr<TFramedTransport> smTrans(new TFramedTransport(smSock));
        boost::shared_ptr<TProtocol> smProto(new TBinaryProtocol(smTrans));
        smClients[i].second.reset(new fpi::TestSMSvcClient(smProto));
        /* Connect to SM by sending my ip port so sm can connect to me */
        smTrans->open();
        smClients[i].first = smClients[i].second->associate(myIp, amPort);
        std::cout << "started connection. Id : " << smClients[i].first << "\n";
    }

    sleep(5);

    /* To generate random data */
    auto datagen = boost::make_shared<RandDataGenerator<>>(4096, 4096);
    auto putMsgGenF = std::bind(&SvcMsgFactory::newPutObjectMsg, volId, datagen);
    /* To geenrate put messages and cache them.  Wraps datagen from above */
    auto putMsgGen = boost::make_shared<
                    CachedMsgGenerator<fpi::PutObjectMsg>>(dataCacheSz,
                                                           true,
                                                           putMsgGenF);
    /* Start google profiler */
    // ProfilerStart("/tmp/output.prof");

    /* Start timer */
    startTs_ = util::getTimeStampNanos();

    /* Issue puts */
    for (int i = 0; i < nPuts; i++) {
        int clientIdx = i % smClients.size();
        auto putObjMsg = putMsgGen->nextItem();
        fpi::AsyncHdr hdr;
        hdr.msg_src_id = smClients[clientIdx].first;
        putsIssued_++;
        smClients[clientIdx].second->putObject(hdr, *putObjMsg);
    }

    /* Poll for completion */
    POLL_MS((putsIssued_ == putsSuccessCnt_ + putsFailedCnt_), 500, (nPuts+2) * 1000);

    /* End profiler */
    // ProfilerStop();

    std::cout << "Total Time taken: " << endTs_ - startTs_ << "(ns)\n"
            << "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / putsIssued_
            << "(ns)\n";
    ASSERT_TRUE(putsIssued_ == putsSuccessCnt_) << "putsIssued: " << putsIssued_
        << " putsSuccessCnt_: " << putsSuccessCnt_
        << " putsFailedCnt_: " << putsFailedCnt_;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("conn-cnt", po::value<int>()->default_value(1), "stream count")
        ("puts-cnt", po::value<int>()->default_value(10), "puts count")
        ("sm-ip", po::value<std::string>()->default_value("127.0.0.1"), "sm-ip")
        ("sm-port", po::value<int>()->default_value(9092), "sm port")
        ("my-ip", po::value<std::string>()->default_value("127.0.0.1"), "my ip")
        ("am-port", po::value<int>()->default_value(9094), "am port");
    AMTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

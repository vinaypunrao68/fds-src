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
#include <thrift/server/TNonblockingServer.h>
#include <ObjectId.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include "TestAMSvc.h"
#include "TestSMSvc.h"
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

struct AMTest : BaseTestFixture
{
    AMTest();
    virtual ~AMTest();

    void printOpTs(const std::string fileName);
    void serve();

    std::atomic<uint32_t> putsIssued_;
    std::atomic<uint32_t> putsSuccessCnt_;
    std::atomic<uint32_t> putsFailedCnt_;
    std::atomic<uint32_t> outStanding_;
    Monitor monitor_;
    bool waiting_;
    uint32_t concurrency_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
    std::vector<SvcRequestIf::SvcReqTs> opTs_;

    std::thread *t_;
    boost::shared_ptr<TServer> server_;
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
        amTest_->outStanding_--;
        {
            Synchronized s(amTest_->monitor_);
            if (amTest_->waiting_ && amTest_->outStanding_ < amTest_->concurrency_) {
                amTest_->monitor_.notify();
            }
        }

        amTest_->opTs_[asyncHdr->msg_src_id].rspRcvdTs =
            amTest_->opTs_[asyncHdr->msg_src_id].rspHndlrTs = util::getTimeStampNanos();
        amTest_->opTs_[asyncHdr->msg_src_id].rqRcvdTs = asyncHdr->rqRcvdTs;
        amTest_->opTs_[asyncHdr->msg_src_id].rqHndlrTs = asyncHdr->rqHndlrTs;
        amTest_->opTs_[asyncHdr->msg_src_id].rspSerStartTs = asyncHdr->rspSerStartTs;
        amTest_->opTs_[asyncHdr->msg_src_id].rspSendStartTs = asyncHdr->rspSendStartTs;

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
        std::cout << "Update catalog resp\n";
    }

 protected:
    AMTest *amTest_;
};


AMTest::AMTest()
{
    putsIssued_ = 0;
    putsSuccessCnt_ = 0;
    putsFailedCnt_ = 0;
    outStanding_ = 0;
    concurrency_ = 0;
    waiting_ = false;

    int amPort = this->getArg<int>("am-port");
    boost::shared_ptr<TestAMSvcHandler> handler(new TestAMSvcHandler(this));
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    boost::shared_ptr<TProcessor> processor(
        new ::FDS_ProtocolInterface::TestAMSvcProcessor(handler));
    if (this->getArg<std::string>("server") == "threaded") {
        boost::shared_ptr<TServerTransport> serverTransport;
        if (this->getArg<bool>("use-pipe")) {
            serverTransport.reset(new TServerSocket("/tmp/amtest"));
        } else {
            serverTransport.reset(new TServerSocket(amPort));
        }
        boost::shared_ptr<TTransportFactory> transportFactory;
        if (this->getArg<std::string>("transport") == "framed") {
            transportFactory.reset(new TFramedTransportFactory());
        } else {
            transportFactory.reset(new TBufferedTransportFactory());
        }
        server_.reset(new TThreadedServer(processor, serverTransport,
                                          transportFactory, protocolFactory));
        std::cout << "Threaded Server init at port: " << amPort << std::endl;
    } else {
        boost::shared_ptr<ThreadManager> threadManager =
            ThreadManager::newSimpleThreadManager(5);
        boost::shared_ptr<PosixThreadFactory> threadFactory =
            boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
        threadManager->threadFactory(threadFactory);
        threadManager->start();
        server_.reset(new TNonblockingServer(processor,
                                             protocolFactory, amPort, threadManager));
        std::cout << "Nonblocking Server init at port: " << amPort << std::endl;
    }
}

void AMTest::serve()
{
    t_ = new std::thread(std::bind(&TServer::serve, server_.get()));
}

AMTest::~AMTest()
{
    server_->stop();
    t_->join();
}

void AMTest::printOpTs(const std::string fileName) {
    std::ofstream o(fileName);
    for (auto &t : opTs_) {
        fds_verify(t.rqSendStartTs != 0 && t.rqSendEndTs != 0);
        o << t.rqStartTs << "\t"
            << t.rqSendStartTs << "\t"
            << t.rqSendEndTs << "\t"
            << t.rqRcvdTs << "\t"
            << t.rqHndlrTs << "\t"
            << t.rspSerStartTs << "\t"
            << t.rspSendStartTs << "\t"
            << t.rspRcvdTs << "\t"
            << t.rspHndlrTs << "\n";
    }
    o.close();
    std::cout << "||st-snd|"
        << "rq-snd|"
        << "rq-snd-rcv|"
        << "rq-rcv-hndlr|"
        << "rsp-hndlr-ser|"
        << "rsp-ser|"
        << "rsp-snd-rcv|"
        << "rsp-rcv-hndlr|"
        << "total||\n";

    for (uint32_t i = 0; i < opTs_.size(); i+=100) {
        auto &t = opTs_[i];
        std::cout << "|"
            << t.rqSendStartTs - t.rqStartTs << "|"
            << t.rqSendEndTs - t.rqSendStartTs << "|"
            << t.rqRcvdTs - t.rqSendEndTs << "|"
            << t.rqHndlrTs - t.rqRcvdTs << "|"
            << t.rspSerStartTs - t.rqHndlrTs << "|"
            << t.rspSendStartTs - t.rspSerStartTs << "|"
            << t.rspRcvdTs - t.rspSendStartTs << "|"
            << t.rspHndlrTs - t.rspRcvdTs << "|"
            << t.rspHndlrTs - t.rqStartTs
            << "|\n";
    }

    /* Compute averages */
    uint64_t st_snd_avg = 0;
    uint64_t rq_snd_avg = 0;
    uint64_t rq_snd_rcv_avg = 0;
    uint64_t rq_rcv_hndlr_avg = 0;
    uint64_t rsp_hndlr_ser_avg = 0;
    uint64_t rsp_ser_avg = 0;
    uint64_t rsp_snd_rcv_avg = 0;
    uint64_t rsp_rcv_hndlr_avg = 0;
    uint64_t total_avg = 0;
    for (auto &t : opTs_) {
        st_snd_avg += (t.rqSendStartTs - t.rqStartTs);
        rq_snd_avg += (t.rqSendEndTs - t.rqSendStartTs);
        rq_snd_rcv_avg += (t.rqRcvdTs - t.rqSendEndTs);
        rq_rcv_hndlr_avg += (t.rqHndlrTs - t.rqRcvdTs);
        rsp_hndlr_ser_avg += (t.rspSerStartTs - t.rqHndlrTs);
        rsp_ser_avg += (t.rspSendStartTs - t.rspSerStartTs);
        rsp_snd_rcv_avg += (t.rspRcvdTs - t.rspSendStartTs);
        rsp_rcv_hndlr_avg += (t.rspHndlrTs - t.rspRcvdTs);
        total_avg += (t.rspHndlrTs - t.rqStartTs);
    }
    std::cout << "|"
        << st_snd_avg / opTs_.size() << "|"
        << rq_snd_avg / opTs_.size() << "|"
        << rq_snd_rcv_avg / opTs_.size() << "|"
        << rq_rcv_hndlr_avg / opTs_.size() << "|"
        << rsp_hndlr_ser_avg / opTs_.size() << "|"
        << rsp_ser_avg / opTs_.size() << "|"
        << rsp_snd_rcv_avg / opTs_.size() << "|"
        << rsp_rcv_hndlr_avg / opTs_.size() << "|"
        << total_avg / opTs_.size()
        << "|" << std::endl;
}

TEST_F(AMTest, smtest)
{
    int dataCacheSz = 100;
    int connCnt = this->getArg<int>("conn-cnt");
    int nPuts =  this->getArg<int>("puts-cnt");
    std::string smIp = this->getArg<std::string>("sm-ip");
    std::string myIp = this->getArg<std::string>("my-ip");
    int smPort = this->getArg<int>("sm-port");
    int amPort = this->getArg<int>("am-port");
    bool framed = (this->getArg<std::string>("transport") == "framed");
    bool pipe = this->getArg<bool>("use-pipe");
    concurrency_ = this->getArg<uint32_t>("concurrency");
    if (concurrency_ == 0) {
        concurrency_  = nPuts;
    }
    if (pipe == true) {
        smIp = "/tmp/smtest";
        myIp = "/tmp/amtest";
    }
    std::cout << "Concurrency level: " << concurrency_ << std::endl;

    opTs_.resize(nPuts);

    int volId = 0;
    std::vector<std::pair<int, boost::shared_ptr<fpi::TestSMSvcClient>>> smClients(connCnt);

    /* start server for responses */
    serve();

    /* Wait a bit */
    sleep(2);

    /* Create connections against SM */
    for (int i = 0; i < connCnt; i++) {
        boost::shared_ptr<TTransport> smSock;
        if (this->getArg<bool>("use-pipe")) {
            smSock.reset(new TSocket(smIp));
        } else {
            smSock.reset(new TSocket(smIp, smPort));
        }
        boost::shared_ptr<TTransport> smTrans;
        if (framed) {
            smTrans.reset(new TFramedTransport(smSock));
        } else {
            smTrans.reset(new TBufferedTransport(smSock));
        }
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
        hdr.msg_src_id = i;
        hdr.msg_src_uuid.svc_uuid = smClients[clientIdx].first;
        putsIssued_++;
        outStanding_++;
        opTs_[i].rqStartTs = opTs_[i].rqSendStartTs = util::getTimeStampNanos();
        smClients[clientIdx].second->putObject(hdr, *putObjMsg);
        opTs_[i].rqSendEndTs = util::getTimeStampNanos();
        {
            Synchronized s(monitor_);
            while (outStanding_ >= concurrency_) {
                waiting_ = true;
                monitor_.wait();
            }
            waiting_ = false;
        }
    }

    /* Poll for completion */
    POLL_MS((putsIssued_ == putsSuccessCnt_ + putsFailedCnt_), 500, (nPuts+2) * 1000);

    /* End profiler */
    // ProfilerStop();

    double throughput = (static_cast<double>(1000000000) / (endTs_ - startTs_)) * putsIssued_;
    double sendThroughput = (static_cast<double>(1000000000) /
                             (opTs_[opTs_.size()-1].rqRcvdTs - opTs_[0].rqStartTs)) * opTs_.size();
    std::cout << "Total Time taken: " << endTs_ - startTs_ << "(ns)\n"
            << "Send throughput: " << sendThroughput
            << "Throughput: " << throughput << std::endl
            << "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / putsIssued_
            << "(ns)\n";
    printOpTs(this->getArg<std::string>("output"));
    ASSERT_TRUE(putsIssued_ == putsSuccessCnt_) << "putsIssued: " << putsIssued_
        << " putsSuccessCnt_: " << putsSuccessCnt_
        << " putsFailedCnt_: " << putsFailedCnt_;
}

TEST_F(AMTest, DISABLED_dmtest)
{
    int dataCacheSz = 100;
    int connCnt = this->getArg<int>("conn-cnt");
    int nPuts =  this->getArg<int>("puts-cnt");
    std::string dmIp = this->getArg<std::string>("dm-ip");
    std::string myIp = this->getArg<std::string>("my-ip");
    int dmPort = this->getArg<int>("dm-port");
    int amPort = this->getArg<int>("am-port");

    int volId = 0;
    std::vector<std::pair<int, boost::shared_ptr<fpi::TestDMSvcClient>>> dmClients(connCnt);

    /* start server for responses */
    serve();

    /* Wait a bit */
    sleep(2);

    /* Create connections against DM */
    for (int i = 0; i < connCnt; i++) {
        boost::shared_ptr<TTransport> dmSock(new TSocket(dmIp, dmPort));
        boost::shared_ptr<TFramedTransport> dmTrans(new TFramedTransport(dmSock));
        boost::shared_ptr<TProtocol> dmProto(new TBinaryProtocol(dmTrans));
        dmClients[i].second.reset(new fpi::TestDMSvcClient(dmProto));
        /* Connect to DM by sending my ip port so dm can connect to me */
        dmTrans->open();
        dmClients[i].first = dmClients[i].second->associate(myIp, amPort);
        std::cout << "started connection. Id : " << dmClients[i].first << "\n";
    }

    sleep(5);

    fpi::AsyncHdr hdr;
    fpi::UpdateCatalogOnceMsg upcat;
    hdr.msg_src_uuid.svc_uuid = dmClients[0].first;
    dmClients[0].second->updateCatalog(hdr, upcat);

    sleep(5);
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
        ("dm-ip", po::value<std::string>()->default_value("127.0.0.1"), "dm-ip")
        ("dm-port", po::value<int>()->default_value(9097), "dm port")
        ("my-ip", po::value<std::string>()->default_value("127.0.0.1"), "my ip")
        ("am-port", po::value<int>()->default_value(9094), "am port")
        ("server", po::value<std::string>()->default_value("threaded"), "Server type")
        ("transport", po::value<std::string>()->default_value("framed"), "transport type")
        ("output", po::value<std::string>()->default_value("stats.txt"), "stats output")
        ("use-pipe", po::value<bool>()->default_value(false), "use pipe transport")
        ("concurrency", po::value<uint32_t>()->default_value(0), "concurrency");
    AMTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

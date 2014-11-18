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
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include "TestAMSvc.h"
#include "TestSMSvc.h"
#include "TestDMSvc.h"
#include <concurrency/LFThreadpool.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/profiler.h>
// #define RECORD_TS(statement) if (recordTs) { statement; }
using ::testing::AtLeast;
using ::testing::Return;

using namespace fds;  // NOLINT
using namespace apache::thrift; // NOLINT
using namespace apache::thrift::protocol; // NOLINT
using namespace apache::thrift::transport; // NOLINT
using namespace apache::thrift::server; // NOLINT
using namespace apache::thrift::concurrency; // NOLINT

const uint32_t BUFFER_SZ = (1 * 1024 * 1024);
bool recordTs = true;
std::vector<SvcRequestIf::SvcReqTs> *opTs;
uint32_t sendCntr= 0;
uint32_t recvCntr= 0;
util::TimeStamp startTs = 0;
util::TimeStamp endTs = 0;

struct BaseServer;
struct SMHandler;
struct AMHandler;

template<class PayloadT>
uint32_t myserialize(const PayloadT &payload,
                     boost::shared_ptr<tt::TMemoryBuffer> &memBuffer)
{
    boost::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(memBuffer));

    uint32_t written = payload.write(binary_buf.get());
    return written;
}

template<class PayloadT>
uint32_t mydeserialize(uint8_t *buf, uint32_t bufSz, PayloadT& payload) {
    boost::shared_ptr<tt::TMemoryBuffer> memory_buf(new tt::TMemoryBuffer(buf, bufSz));
    boost::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(memory_buf));

    uint32_t read = payload.read(binary_buf.get());
    return read;
}

struct AMTest : BaseTestFixture
{
    AMTest();
    virtual ~AMTest();

    void printOpTs(const std::string fileName);
    void serve();

    std::atomic<uint32_t> putsIssued_;
    std::atomic<uint32_t> putsSuccessCnt_;
    std::atomic<uint32_t> putsFailedCnt_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
    LatencyCounter avgPutLatency_;
    std::vector<SvcRequestIf::SvcReqTs> opTs_;
    Monitor monitor_;
    bool waiting_;
    uint32_t concurrency_;
    std::atomic<uint32_t> outStanding_;

    std::thread *smThread_;
    boost::shared_ptr<BaseServer> smServer_;
    boost::shared_ptr<SMHandler> smHandler_;
    boost::shared_ptr<TTransport> smSock_;

    std::thread *amThread_;
    boost::shared_ptr<BaseServer> amServer_;
    boost::shared_ptr<AMHandler> amHandler_;
    boost::shared_ptr<TTransport> amSock_;
};

struct SynchronizedTransport
{
    SynchronizedTransport(boost::shared_ptr<TTransport> client)
    {
        client_ = client;
        buf_ = new uint8_t[BUFFER_SZ];
        memBuffer_.reset(
            new tt::TMemoryBuffer(buf_, BUFFER_SZ, TMemoryBuffer::TAKE_OWNERSHIP));
    }

    ~SynchronizedTransport() {
        delete [] buf_;
    }

    template<class PayloadT>
    void sendPayload(PayloadT& payload) {
        int dataSz;
        /* Serialize */
        memBuffer_->resetBuffer();
        boost::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(memBuffer_));
        dataSz = payload.write(binary_buf.get());

        /* write data size */
        client_->write(reinterpret_cast<uint8_t*>(&dataSz), sizeof(int));
        /* write data */
        client_->write(buf_, dataSz);
        client_->flush();
    }

    uint8_t *buf_;
    boost::shared_ptr<TTransport> client_;
    boost::shared_ptr<tt::TMemoryBuffer> memBuffer_;
};

struct BaseServerHandler {
    virtual void handle(uint8_t* data, uint32_t sz) = 0;
};

struct SMHandler : BaseServerHandler {
    SMHandler()
    : tp_(1)
    {}

    void setAmClient(boost::shared_ptr<TTransport> client) {
        amClient_.reset(new SynchronizedTransport(client));
    }

    void respondToAm() {
        fpi::PutObjectRspMsg resp;
        amClient_->sendPayload(resp);
    }

    virtual void handle(uint8_t* data, uint32_t dataSz)
    {
        /* Hardcoding to deserialize PutObject */
        // TODO(Rao):
        fpi::PutObjectMsgPtr putObj(new fpi::PutObjectMsg());
        auto read = mydeserialize(data, dataSz, *putObj);
        // std::cout << "deser sz: " << read << std::endl;
        fds_verify(read == dataSz);
        tp_.schedule(&SMHandler::respondToAm, this);

        delete [] data;
    }


    LFMQThreadpool tp_;
    boost::shared_ptr<SynchronizedTransport> amClient_;
};

struct AMHandler : BaseServerHandler {
    AMHandler(AMTest *amTest)
    {
        amTest_ = amTest;
    }
    virtual void handle(uint8_t* data, uint32_t dataSz)
    {
        delete [] data;

        amTest_->opTs_[recvCntr].rspHndlrTs = util::getTimeStampNanos();
        amTest_->avgPutLatency_.update(
            amTest_->opTs_[recvCntr].rspHndlrTs - amTest_->opTs_[recvCntr].rqStartTs);

        amTest_->outStanding_--;
        {
            Synchronized s(amTest_->monitor_);
            if (amTest_->waiting_ && amTest_->outStanding_ < amTest_->concurrency_) {
                amTest_->monitor_.notify();
            }
        }
        recvCntr++;
        if (sendCntr == recvCntr) {
            endTs = util::getTimeStampNanos();
        }
    }

    AMTest *amTest_;
};

struct BaseServerTask {
    BaseServerTask(boost::shared_ptr<TTransport> client,
                   boost::shared_ptr<BaseServerHandler> handler);
    ~BaseServerTask();
    void start();
    void stop();
    void run();
    
    boost::shared_ptr<TTransport> client_;
    boost::shared_ptr<BaseServerHandler> handler_;
    std::unique_ptr<std::thread> thread_;
    std::unique_ptr<uint8_t> buf_;
};


BaseServerTask::BaseServerTask(boost::shared_ptr<TTransport> client, boost::shared_ptr<BaseServerHandler> handler)
    : client_(client),
    handler_(handler)
{
    buf_.reset(new uint8_t[BUFFER_SZ]);
}

BaseServerTask::~BaseServerTask()
{
    stop();
}

void BaseServerTask::start()
{
    thread_.reset(new std::thread(&BaseServerTask::run, this));
}

void BaseServerTask::stop()
{
    client_->close();
    thread_->join();
}

void BaseServerTask::run() {
    uint32_t cntr = 0;
    try {
#if 0
        uint8_t* data = reinterpret_cast<uint8_t*>(
                const_cast<uint8_t*>(buf_.get()));
#endif
        for (;;) {
            uint32_t nbytes = 0;
            /* Reaad the frame size */
            int dataSz;
            nbytes = client_->readAll(reinterpret_cast<uint8_t*>(&dataSz), sizeof(dataSz));
            fds_verify(nbytes == sizeof(dataSz));

            /* Read the frame */
            uint8_t* data = new uint8_t[dataSz];
            nbytes = client_->readAll(data, dataSz);
            fds_verify(static_cast<int>(nbytes) == dataSz);

            handler_->handle(data, nbytes);

#if 0
            RECORD_TS((*opTs)[cntr].rqRcvdTs = util::getTimeStampNanos())
            RECORD_TS((*opTs)[cntr].rqHndlrTs = (*opTs)[cntr].rqRcvdTs)
            RECORD_TS((*opTs)[cntr].rspSerStartTs = (*opTs)[cntr].rqRcvdTs)
            RECORD_TS((*opTs)[cntr].rspSendStartTs = (*opTs)[cntr].rqRcvdTs)
            RECORD_TS((*opTs)[cntr].rspRcvdTs = (*opTs)[cntr].rqRcvdTs)
            RECORD_TS((*opTs)[cntr].rspHndlrTs = (*opTs)[cntr].rqRcvdTs)

            recvCntr++;
            if (sendCntr == recvCntr) {
                endTs = util::getTimeStampNanos();
            }
            dispatch_(data, dataSz); 
#endif
        }
    } catch (const TTransportException& ttx) {
        if (ttx.getType() != TTransportException::END_OF_FILE) {
            std::string errStr = std::string("TThreadedServer client died: ") + ttx.what();
        }
    } catch (const std::exception &x) {
    } catch (...) {
    }

    /* Remove the task */
    // TODO(Rao):
}


struct BaseServer : public TServer {
    BaseServer(boost::shared_ptr<TServerTransport> serverTransport,
               boost::shared_ptr<BaseServerHandler> handler)
    : TServer(boost::shared_ptr<TProcessorFactory>(nullptr), serverTransport),
    stop_(false)
    {
        handler_ = handler;
    }
    virtual void serve()
    {
        boost::shared_ptr<TTransport> client;

        serverTransport_->listen();
        while (!stop_) {
            try {
                client.reset();

                // Fetch client from server
                client = serverTransport_->accept();
                std::cout << "Received new connection request\n";

                // Create task and insert into tasks
                BaseServerTask* task = new BaseServerTask(client, handler_);
                {
                    Synchronized s(tasksMonitor_);
                    tasks_.insert(task);
                }
                // Start the thread!
                task->start();

            } catch (TTransportException& ttx) {
                if (client != NULL) { client->close(); }
                if (!stop_ || ttx.getType() != TTransportException::INTERRUPTED) {
                    std::string errStr = std::string("TThreadedServer: TServerTransport died on accept: ") + ttx.what();
                    GlobalOutput(errStr.c_str());
                }
                continue;
            } catch (TException& tx) {
                if (client != NULL) { client->close(); }
                std::string errStr = std::string("TThreadedServer: Caught TException: ") + tx.what();
                GlobalOutput(errStr.c_str());
                continue;
            }
        }

        // If stopped manually, make sure to close server transport
        if (stop_) {
            try {
                serverTransport_->close();
            } catch (TException &tx) {
                std::string errStr = std::string("TThreadedServer: Exception shutting down: ") + tx.what();
                GlobalOutput(errStr.c_str());
            }
            try {
                for (auto *t : tasks_) {
                    t->stop();
                }
            } catch (TException &tx) {
                std::string errStr = std::string("TThreadedServer: Exception joining workers: ") + tx.what();
                GlobalOutput(errStr.c_str());
            }
        }
    }
    virtual void stop()
    {
        stop_ = true;
        serverTransport_->interrupt();
    }

    bool stop_;
    Monitor tasksMonitor_;
    std::set<BaseServerTask*> tasks_;
    boost::shared_ptr<BaseServerHandler> handler_;
};


AMTest::AMTest()
{
    outStanding_ = 0;
    concurrency_ = 0;
    waiting_ = false;
    putsIssued_ = 0;
    putsSuccessCnt_ = 0;
    putsFailedCnt_ = 0;

    int smPort = this->getArg<int>("sm-port");
    boost::shared_ptr<TServerTransport> smServerTransport(new TServerSocket(smPort));
    smHandler_.reset(new SMHandler());
    smServer_.reset(new BaseServer(smServerTransport, smHandler_));
    std::cout << "SM init at port: " << smPort << std::endl;

    int amPort = this->getArg<int>("am-port");
    boost::shared_ptr<TServerTransport> amServerTransport(new TServerSocket(amPort));
    amHandler_.reset(new AMHandler(this));
    amServer_.reset(new BaseServer(amServerTransport, amHandler_));
    std::cout << "AM init at port: " << amPort << std::endl;
}

void AMTest::serve()
{
    smThread_ = new std::thread(std::bind(&BaseServer::serve, smServer_.get()));
    amThread_ = new std::thread(std::bind(&BaseServer::serve, amServer_.get()));
    sleep(5);

    /* Establish connection against sm and am */
    smSock_.reset(new TSocket("127.0.0.1", this->getArg<int>("sm-port")));
    smSock_->open();
    std::cout << "started connection against sm\n";

    amSock_.reset(new TSocket("127.0.0.1", this->getArg<int>("am-port")));
    amSock_->open();
    std::cout << "started connection against am\n";

    smHandler_->setAmClient(amSock_);

    sleep(2);
}

AMTest::~AMTest()
{
    smServer_->stop();
    smThread_->join();
    amServer_->stop();
    amThread_->join();
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
    uint32_t nPuts =  this->getArg<uint32_t>("puts-cnt");
    uint32_t dataCacheSz = 100;
    uint64_t volId = 1;
    concurrency_ = this->getArg<uint32_t>("concurrency");
    if (concurrency_ == 0) {
        concurrency_  = nPuts;
    }
    opTs_.resize(nPuts);

    /* start server for responses */
    serve();
    SynchronizedTransport smTrans(smSock_);

    /* To generate random data */
    auto datagen = boost::make_shared<RandDataGenerator<>>(4096, 4096);
    auto putMsgGenF = std::bind(&SvcMsgFactory::newPutObjectMsg, volId, datagen);
    /* To geenrate put messages and cache them.  Wraps datagen from above */
    auto putMsgGen = boost::make_shared<
                    CachedMsgGenerator<fpi::PutObjectMsg>>(dataCacheSz,
                                                           true,
                                                           putMsgGenF);
    /* Start timer */
    startTs = util::getTimeStampNanos();

    /* Issue puts */
    sendCntr = nPuts;
    for (uint32_t i = 0; i < nPuts; i++) {
        auto putObjMsg = putMsgGen->nextItem();
        outStanding_++;
        opTs_[i].rqStartTs = util::getTimeStampNanos();
        smTrans.sendPayload(*putObjMsg);

        {
            Synchronized s(monitor_);
            while (outStanding_ >= concurrency_) {
                waiting_ = true;
                monitor_.wait();
            }
            waiting_ = false;
        }
    }

    sleep(5);
    fds_verify(recvCntr == nPuts);
    double throughput = (static_cast<double>(1000000000) / (endTs - startTs)) * nPuts;
    std::cout << "start: " << startTs << "end: " << endTs << std::endl;
    std::cout << "Avg time: " << (endTs - startTs) / nPuts;
    std::cout << "throuput: " << throughput << std::endl;
    std::cout << "Avg put latency: " << avgPutLatency_.value() << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<uint32_t>()->default_value(10000), "puts count")
        ("sm-ip", po::value<std::string>()->default_value("127.0.0.1"), "sm-ip")
        ("sm-port", po::value<int>()->default_value(9092), "sm port")
        ("am-port", po::value<int>()->default_value(9094), "am port")
        ("output", po::value<std::string>()->default_value("stats.txt"), "stats output")
        ("concurrency", po::value<uint32_t>()->default_value(0), "concurrency");
    AMTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

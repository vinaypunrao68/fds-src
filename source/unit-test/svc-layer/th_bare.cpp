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

const uint32_t BUFFER_SZ = (1 * 1024 * 1024);
std::vector<SvcRequestIf::SvcReqTs> *opTs;

struct BaseServer;

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

struct BaseServerTask {
    BaseServerTask(boost::shared_ptr<TTransport> client);
    ~BaseServerTask();
    void start();
    void stop();
    void run();
    
    void dispatch_(uint8_t *data, uint32_t dataSz);

    boost::shared_ptr<TTransport> client_;
    std::unique_ptr<std::thread> thread_;
    std::unique_ptr<uint8_t> buf_;
};

BaseServerTask::BaseServerTask(boost::shared_ptr<TTransport> client)
    : client_(client)
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
        uint8_t* data = reinterpret_cast<uint8_t*>(
                const_cast<uint8_t*>(buf_.get()));
        for (;;) {
            uint32_t nbytes = 0;
            /* Reaad the frame size */
            int dataSz;
            nbytes = client_->readAll(reinterpret_cast<uint8_t*>(&dataSz), sizeof(dataSz));
            fds_verify(nbytes == sizeof(dataSz));

            /* Read the frame */
            nbytes = client_->readAll(data, dataSz);
            fds_verify(static_cast<int>(nbytes) == dataSz);

            (*opTs)[cntr].rqRcvdTs = util::getTimeStampNanos();
            (*opTs)[cntr].rqHndlrTs = (*opTs)[cntr].rqRcvdTs;
            (*opTs)[cntr].rspSerStartTs = (*opTs)[cntr].rqRcvdTs;
            (*opTs)[cntr].rspSendStartTs = (*opTs)[cntr].rqRcvdTs;
            (*opTs)[cntr].rspRcvdTs = (*opTs)[cntr].rqRcvdTs;
            (*opTs)[cntr].rspHndlrTs = (*opTs)[cntr].rqRcvdTs;

            dispatch_(data, dataSz); 
            cntr++;
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

void BaseServerTask::dispatch_(uint8_t *data, uint32_t dataSz)
{
    /* Hardcoding to deserialize PutObject */
    // TODO(Rao):
    fpi::PutObjectMsgPtr putObj(new fpi::PutObjectMsg());
    auto read = mydeserialize(data, dataSz, *putObj);
    std::cout << "deser sz: " << read << std::endl;
    fds_verify(read == dataSz);
}

struct BaseServer : public TServer {
    BaseServer(boost::shared_ptr<TServerTransport> serverTransport)
    : TServer(boost::shared_ptr<TProcessorFactory>(nullptr), serverTransport),
    stop_(false)
    {
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
                BaseServerTask* task = new BaseServerTask(client);
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
};

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
    std::vector<SvcRequestIf::SvcReqTs> opTs_;

    std::thread *t_;
    boost::shared_ptr<BaseServer> server_;
};

AMTest::AMTest()
{
    putsIssued_ = 0;
    putsSuccessCnt_ = 0;
    putsFailedCnt_ = 0;

    int amPort = this->getArg<int>("am-port");
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(amPort));
    server_.reset(new BaseServer(serverTransport));
    std::cout << "Server init at port: " << amPort << std::endl;
}

void AMTest::serve()
{
    t_ = new std::thread(std::bind(&BaseServer::serve, server_.get()));
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
    int nPuts =  this->getArg<int>("puts-cnt");
    int amPort = this->getArg<int>("am-port");
    uint32_t dataCacheSz = 100;
    uint64_t volId = 1;
    opTs_.resize(nPuts);
    opTs = &opTs_;

    /* start server for responses */
    serve();

    /* Wait a bit */
    sleep(2);

    /* Start a client connection */
    boost::shared_ptr<TTransport> smSock(new TSocket("127.0.0.1", amPort));
    smSock->open();
    std::cout << "started connection against sm\n";

    sleep(5);

    /* To generate random data */
    auto datagen = boost::make_shared<RandDataGenerator<>>(4096, 4096);
    auto putMsgGenF = std::bind(&SvcMsgFactory::newPutObjectMsg, volId, datagen);
    /* To geenrate put messages and cache them.  Wraps datagen from above */
    auto putMsgGen = boost::make_shared<
                    CachedMsgGenerator<fpi::PutObjectMsg>>(dataCacheSz,
                                                           true,
                                                           putMsgGenF);
    uint8_t *buf = new uint8_t[BUFFER_SZ];
    boost::shared_ptr<tt::TMemoryBuffer> memBuffer(
        new tt::TMemoryBuffer(buf, BUFFER_SZ, TMemoryBuffer::TAKE_OWNERSHIP));
    int dataSz;
    /* Start timer */
    startTs_ = util::getTimeStampNanos();

    /* Issue puts */
    for (int i = 0; i < nPuts; i++) {
        opTs_[i].rqStartTs = util::getTimeStampNanos();
        auto putObjMsg = putMsgGen->nextItem();
        /* Serialize */
        memBuffer->resetBuffer();
        boost::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(memBuffer));
        dataSz = putObjMsg->write(binary_buf.get());

        opTs_[i].rqSendStartTs = util::getTimeStampNanos();
        /* write data size */
        smSock->write(reinterpret_cast<uint8_t*>(&dataSz), sizeof(int));
        /* write data */
        smSock->write(buf, dataSz);
        smSock->flush();
        opTs_[i].rqSendEndTs = util::getTimeStampNanos();
    }

    sleep(5);
    printOpTs("temp.txt");
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
        ("output", po::value<std::string>()->default_value("stats.txt"), "stats output");
    AMTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

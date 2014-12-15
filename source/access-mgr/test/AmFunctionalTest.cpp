/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <string>
#include <vector>
#include <map>
#include <thread>
#include <fdsn-server.h>
#include <util/fds_stat.h>
#include <am-platform.h>
#include <net/net-service.h>
#include <AccessMgr.h>

#include "boost/program_options.hpp"
#include <google/profiler.h>
#include <gtest/gtest.h>
#include <testlib/DataGen.hpp>

namespace fds {

AccessMgr::unique_ptr am;

class AmProcessWrapper : public FdsProcess {
  public:
    AmProcessWrapper(int argc, char * argv[], const std::string & config,
                     const std::string & basePath, Module * vec[])
            : FdsProcess(argc, argv, config, basePath, vec) {
    }

    virtual ~AmProcessWrapper() {
    }
    virtual void proc_pre_startup() override {
        FdsProcess::proc_pre_startup();
        am = AccessMgr::unique_ptr(new AccessMgr("AM Functional Test Module", this));
        proc_add_module(am.get());
        Module *lckstp[] = { am.get(), NULL };
        proc_assign_locksteps(lckstp);
    }

    virtual int run() override {
        return 0;
    }
};

struct null_deleter
{
    void operator()(void const *) const {
    }
};

class AmLoadProc : public AmAsyncResponseApi,
                   public apis::AsyncAmServiceResponseIf {
  public:
    AmLoadProc(int argc, char **argv)
            : domainName(new std::string("Test Domain")),
              volumeName(new std::string("Test Volume")),
              serverIp("127.0.0.1"),
              serverPort(8899),
              responsePort(9876) {
        // register and populate volumes
        VolumeDesc volDesc(*volumeName, 5);
        volDesc.iops_min = 0;
        volDesc.iops_max = 0;
        volDesc.relativePrio = 1;
        fds_verify(am->registerVolume(volDesc) == ERR_OK);

        namespace po = boost::program_options;
        po::options_description desc("AM functional test");
        desc.add_options()
                ("help,h", "Print this help message")
                ("threads,t",
                 po::value<fds_uint32_t>(&concurrency)->default_value(1),
                 "Number of dispatch threads")
                ("num-ops,n",
                 po::value<fds_uint32_t>(&totalOps)->default_value(5000),
                 "Number of operations")
                ("blob-size,s",
                 po::value<fds_uint32_t>(&blobSize)->default_value(4096),
                 "Blob size")
                ("profile,p",
                 po::bool_switch(&profile)->default_value(false),
                 "Run google profiler")
                ("profile-file,f",
                 po::value<std::string>(&profileFile)->default_value("am-profile.prof"),
                 "Profile output file")
                ("thrift,r",
                 po::bool_switch(&thrift)->default_value(false),
                 "Use thrift instead of direct call");

        // hardcoding test config
        po::variables_map vm;
        po::parsed_options parsed =
            po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        po::store(parsed, vm);
        po::notify(vm);
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            exit(0);
        }

        opCount = ATOMIC_VAR_INIT(0);
        threads_.resize(concurrency);
    }
    virtual ~AmLoadProc() {
    }
    typedef std::unique_ptr<AmLoadProc> unique_ptr;

    void init() {
        responseApi.reset(this, null_deleter());
        if (thrift) {
            // Setup the async response server
            serverTransport.reset(new xdi_att::TServerSocket(responsePort));
            // serverTransport.reset(new xdi_att::TServerSocket("/tmp/am-resp-sock"));
            transportFactory.reset(new xdi_att::TFramedTransportFactory());
            protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());
            processor.reset(new apis::AsyncAmServiceResponseProcessor(
                responseApi));
            ttServer.reset(new xdi_ats::TThreadedServer(processor,
                                                        serverTransport,
                                                        transportFactory,
                                                        protocolFactory));
            listen_thread.reset(new boost::thread(&xdi_ats::TThreadedServer::serve,
                                                  ttServer.get()));

            // Setup the async response client
            boost::shared_ptr<xdi_att::TTransport> respSock(
                boost::make_shared<xdi_att::TSocket>(
                    serverIp, serverPort));
            // "/tmp/am-req-sock"));
            boost::shared_ptr<xdi_att::TFramedTransport> respTrans(
                boost::make_shared<xdi_att::TFramedTransport>(respSock));
            boost::shared_ptr<xdi_atp::TProtocol> respProto(
                boost::make_shared<xdi_atp::TBinaryProtocol>(respTrans));
            asyncThriftClient = boost::make_shared<apis::AsyncAmServiceRequestClient>(respProto);
            respSock->open();
            asyncDataApi = boost::dynamic_pointer_cast<apis::AsyncAmServiceRequestIf>(
                asyncThriftClient);
        } else {
            asyncDataApi = boost::make_shared<AmAsyncDataApi>(responseApi);
        }
    }

    enum TaskOps {
        PUT,
        GET,
        STARTTX,
        STAT,
        PUTMETA,
        LISTVOL,
        GETWITHMETA
    };

    // **********
    // Thrift response handlers
    // **********
    void attachVolumeResponse(const apis::RequestId& requestId) {}
    void attachVolumeResponse(boost::shared_ptr<apis::RequestId>& requestId) {}
    void volumeContents(const apis::RequestId& requestId,
                        const std::vector<apis::BlobDescriptor> & response) {}
    void volumeContents(boost::shared_ptr<apis::RequestId>& requestId,
                        boost::shared_ptr<std::vector<apis::BlobDescriptor> >& response) {}
    void statBlobResponse(const apis::RequestId& requestId,
                          const apis::BlobDescriptor& response) {}
    void statBlobResponse(boost::shared_ptr<apis::RequestId>& requestId,
                          boost::shared_ptr<apis::BlobDescriptor>& response) {
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }
    void startBlobTxResponse(const apis::RequestId& requestId,
                             const apis::TxDescriptor& response) {}
    void startBlobTxResponse(boost::shared_ptr<apis::RequestId>& requestId,
                             boost::shared_ptr<apis::TxDescriptor>& response) {}
    void commitBlobTxResponse(const apis::RequestId& requestId) {}
    void commitBlobTxResponse(boost::shared_ptr<apis::RequestId>& requestId) {}
    void abortBlobTxResponse(const apis::RequestId& requestId) {}
    void abortBlobTxResponse(boost::shared_ptr<apis::RequestId>& requestId) {}
    void getBlobResponse(const apis::RequestId& requestId,
                         const std::string& response) {}
    void getBlobResponse(boost::shared_ptr<apis::RequestId>& requestId,
                         boost::shared_ptr<std::string>& response) {
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }
    void getBlobWithMetaResponse(const apis::RequestId& requestId,
                                 const std::string& data,
                                 const apis::BlobDescriptor& blobDesc) {}
    void getBlobWithMetaResponse(boost::shared_ptr<apis::RequestId>& requestId,
                                 boost::shared_ptr<std::string>& data,
                                 boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }
    void handshakeComplete(const apis::RequestId& requestId) {}
    void handshakeComplete(boost::shared_ptr<apis::RequestId>& requestId) {}
    void updateMetadataResponse(const apis::RequestId& requestId) {}
    void updateMetadataResponse(boost::shared_ptr<apis::RequestId>& requestId) {}
    void updateBlobResponse(const apis::RequestId& requestId) {}
    void updateBlobResponse(boost::shared_ptr<apis::RequestId>& requestId) {}
    void updateBlobOnceResponse(const apis::RequestId& requestId) {}
    void updateBlobOnceResponse(boost::shared_ptr<apis::RequestId>& requestId) {}
    void deleteBlobResponse(const apis::RequestId& requestId) {}
    void deleteBlobResponse(boost::shared_ptr<apis::RequestId>& requestId) {}
    void volumeStatus(const apis::RequestId& requestId,
                      const apis::VolumeStatus& response) {}
    void volumeStatus(boost::shared_ptr<apis::RequestId>& requestId,
                      boost::shared_ptr<apis::VolumeStatus>& response) {}
    void completeExceptionally(const apis::RequestId& requestId,
                               const apis::ErrorCode errorCode,
                               const std::string& message) {}
    void completeExceptionally(boost::shared_ptr<apis::RequestId>& requestId,
                               boost::shared_ptr<apis::ErrorCode>& errorCode,
                               boost::shared_ptr<std::string>& message) {}

    void attachVolumeResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void startBlobTxResp(const Error &error,
                         boost::shared_ptr<apis::RequestId>& requestId,
                         boost::shared_ptr<apis::TxDescriptor>& txDesc) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void updateBlobResp(const Error &error,
                        boost::shared_ptr<apis::RequestId>& requestId) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void updateBlobOnceResp(const Error &error,
                            boost::shared_ptr<apis::RequestId>& requestId) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void updateMetadataResp(const Error &error,
                        boost::shared_ptr<apis::RequestId>& requestId) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void abortBlobTxResp(const Error &error,
                         boost::shared_ptr<apis::RequestId>& requestId) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void commitBlobTxResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void getBlobResp(const Error &error,
                     boost::shared_ptr<apis::RequestId>& requestId,
                     boost::shared_ptr<std::string> buf,
                     fds_uint32_t& length) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void getBlobWithMetaResp(const Error &error,
                             boost::shared_ptr<apis::RequestId>& requestId,
                             boost::shared_ptr<std::string> buf,
                             fds_uint32_t& length,
                             boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void statBlobResp(const Error &error,
                      boost::shared_ptr<apis::RequestId>& requestId,
                      boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void deleteBlobResp(const Error &error,
                        boost::shared_ptr<apis::RequestId>& requestId) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void volumeStatusResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId,
                          boost::shared_ptr<apis::VolumeStatus>& volumeStatus) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void volumeContentsResp(const Error &error,
                            boost::shared_ptr<apis::RequestId>& requestId,
                            boost::shared_ptr<std::vector<apis::BlobDescriptor>>& volContents) {
        fds_verify(ERR_OK == error);
        if (totalOps == ++opsDone) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void asyncTask(int id, TaskOps opType) {
        fds_uint32_t ops = atomic_fetch_add(&opCount, (fds_uint32_t)1);
        GLOGDEBUG << "Starting thread " << id;
        // constants we are going to pass to api
        boost::shared_ptr<fds_int32_t> blobMode(new fds_int32_t);
        *blobMode = 1;
        boost::shared_ptr<int32_t> blobLength = boost::make_shared<int32_t>(blobSize);
        boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
        std::map<std::string, std::string> metaData;
        boost::shared_ptr< std::map<std::string, std::string> > meta =
                boost::make_shared<std::map<std::string, std::string>>(metaData);

        SequentialBlobDataGen blobGen(blobSize, id);
        while ((ops + 1) <= totalOps) {
            if (opType == PUT) {
                // Always use an empty request ID since we don't track
                boost::shared_ptr<apis::RequestId> reqId(
                    boost::make_shared<apis::RequestId>());
                // Make copy of data since function "takes" the shared_ptr
                boost::shared_ptr<std::string> localData(
                    boost::make_shared<std::string>(*blobGen.blobData));
                asyncDataApi->updateBlobOnce(reqId,
                                             domainName,
                                             volumeName,
                                             blobGen.blobName,
                                             blobMode,
                                             localData,
                                             blobLength,
                                             off,
                                             meta);
            } else if (opType == GET) {
                // Always use an empty request ID since we don't track
                boost::shared_ptr<apis::RequestId> reqId(
                    boost::make_shared<apis::RequestId>());
                asyncDataApi->getBlob(reqId,
                                      domainName,
                                      volumeName,
                                      blobGen.blobName,
                                      blobLength,
                                      off);
            } else if (opType == GETWITHMETA) {
                // Always use an empty request ID since we don't track
                boost::shared_ptr<apis::RequestId> reqId(
                    boost::make_shared<apis::RequestId>());
                asyncDataApi->getBlobWithMeta(reqId,
                                              domainName,
                                              volumeName,
                                              blobGen.blobName,
                                              blobLength,
                                              off);
            } else if (opType == STARTTX) {
                // Always use an empty request ID since we don't track
                boost::shared_ptr<apis::RequestId> reqId(
                    boost::make_shared<apis::RequestId>());
                asyncDataApi->startBlobTx(reqId,
                                          domainName,
                                          volumeName,
                                          blobGen.blobName,
                                          blobMode);
            } else if (opType == STAT) {
                // Always use an empty request ID since we don't track
                boost::shared_ptr<apis::RequestId> reqId(
                    boost::make_shared<apis::RequestId>());
                asyncDataApi->statBlob(reqId,
                                       domainName,
                                       volumeName,
                                       blobGen.blobName);
            } else if (opType == PUTMETA) {
                // Always use an empty request ID since we don't track
                boost::shared_ptr<apis::RequestId> reqId(
                    boost::make_shared<apis::RequestId>());
                boost::shared_ptr<apis::TxDescriptor> txDesc(
                    boost::make_shared<apis::TxDescriptor>());
                // TODO(Andrew): Setup a test case where this is
                // set done before hand, otherwise these fail
                txDesc->txId = 0;
                asyncDataApi->updateMetadata(reqId,
                                             domainName,
                                             volumeName,
                                             blobGen.blobName,
                                             txDesc,
                                             meta);
            } else if (opType == LISTVOL) {
                // Always use an empty request ID since we don't track
                boost::shared_ptr<apis::RequestId> reqId(
                    boost::make_shared<apis::RequestId>());
                // dummy values for state that we don't use
                boost::shared_ptr<int32_t> count(
                    boost::make_shared<int32_t>());
                *count = 0;
                boost::shared_ptr<int64_t> offset(
                    boost::make_shared<int64_t>());
                *offset = 0;
                asyncDataApi->volumeContents(reqId,
                                             domainName,
                                             volumeName,
                                             count,
                                             offset);
            } else {
                fds_panic("Unknown op type");
            }
            ops = atomic_fetch_add(&opCount, (fds_uint32_t)1);
            blobGen.generateNext();
        }
    }

    void task(int id, TaskOps opType) {
        fds_uint32_t ops = atomic_fetch_add(&opCount, (fds_uint32_t)1);
        GLOGDEBUG << "Starting thread " << id;
        // constants we are going to pass to api
        boost::shared_ptr<fds_int32_t> blobMode(new fds_int32_t);
        *blobMode = 1;
        boost::shared_ptr<int32_t> blobLength = boost::make_shared<int32_t>(blobSize);
        boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
        std::map<std::string, std::string> metaData;
        boost::shared_ptr< std::map<std::string, std::string> > meta =
                boost::make_shared<std::map<std::string, std::string>>(metaData);

        SequentialBlobDataGen blobGen(blobSize, id);
        fds_uint64_t localLatencyTotal = 0;
        fds_uint32_t localOpCount = 0;
        while ((ops + 1) <= totalOps) {
            fds_uint64_t start_nano = util::getTimeStampNanos();
            if (opType == PUT) {
                try {
                    // Make copy of data since function "takes" the shared_ptr
                    boost::shared_ptr<std::string> localData(
                        boost::make_shared<std::string>(*blobGen.blobData));
                    am->dataApi->updateBlobOnce(domainName, volumeName,
                                                blobGen.blobName, blobMode,
                                                localData, blobLength, off, meta);
                } catch(apis::ApiException fdsE) {
                    fds_panic("updateBlob failed");
                }
            } else if (opType == GET) {
                try {
                    std::string data;
                    am->dataApi->getBlob(data,
                                         domainName,
                                         volumeName,
                                         blobGen.blobName,
                                         blobLength,
                                         off);
                } catch(apis::ApiException fdsE) {
                    fds_panic("getBlob failed");
                }
            } else if (opType == STARTTX) {
                try {
                    apis::TxDescriptor txDesc;
                    am->dataApi->startBlobTx(txDesc,
                                             domainName,
                                             volumeName,
                                             blobGen.blobName,
                                             blobMode);
                } catch(apis::ApiException fdsE) {
                    fds_panic("statBlob failed");
                }
            } else {
                fds_panic("Unknown op type");
            }
            localLatencyTotal += (util::getTimeStampNanos() - start_nano);
            ++localOpCount;

            ops = atomic_fetch_add(&opCount, (fds_uint32_t)1);
            blobGen.generateNext();
        }
        avgLatencyTotal += (localLatencyTotal / localOpCount);
    }

    virtual int runTask(TaskOps opType) {
        opCount = 0;
        avgLatencyTotal = 0;
        fds_uint64_t start_nano = util::getTimeStampNanos();
        for (unsigned i = 0; i < concurrency; ++i) {
            std::thread* new_thread = new std::thread(&AmLoadProc::task, this, i, opType);
            threads_[i] = new_thread;
        }

        // Wait for all threads
        for (unsigned x = 0; x < concurrency; ++x) {
            threads_[x]->join();
        }

        fds_uint64_t duration_nano = util::getTimeStampNanos() - start_nano;
        double time_sec = duration_nano / 1000000000.0;
        if (time_sec < 10) {
            std::cout << "Experiment ran for too short time to calc IOPS" << std::endl;
            GLOGNOTIFY << "Experiment ran for too short time to calc IOPS";
        } else {
            std::cout << "Average IOPS = " << totalOps / time_sec << std::endl;
            GLOGNOTIFY << "Average IOPS = " << totalOps / time_sec;
            std::cout << "Average latency = " << avgLatencyTotal / concurrency
                      << " nanosec" << std::endl;
            GLOGNOTIFY << "Average latency = " << avgLatencyTotal / concurrency
                       << " nanosec";
        }

        for (unsigned x = 0; x < concurrency; ++x) {
            std::thread* th = threads_[x];
            delete th;
            threads_[x] = NULL;
        }

        return 0;
    }

    virtual int runAsyncTask(TaskOps opType) {
        opCount = 0;
        opsDone = 0;
        asyncStartNano = util::getTimeStampNanos();
        if (profile) ProfilerStart(profileFile.c_str());
        for (unsigned i = 0; i < concurrency; ++i) {
            std::thread* new_thread = new std::thread(&AmLoadProc::asyncTask, this, i, opType);
            threads_[i] = new_thread;
        }

        // Wait for all threads
        for (unsigned x = 0; x < concurrency; ++x) {
            threads_[x]->join();
        }

        // Wait for responses
        std::unique_lock<std::mutex> lk(done_mutex);
        done_cond.wait_for(lk,
                           std::chrono::milliseconds(1000000),
                           [this](){return totalOps == opsDone;});

        fds_uint64_t duration_nano = asyncStopNano - asyncStartNano;
        if (profile) ProfilerStop();
        double time_sec = duration_nano / 1000000000.0;
        if (time_sec < 10) {
            std::cout << "Experiment ran for too short time to calc IOPS" << std::endl;
            GLOGNOTIFY << "Experiment ran for too short time to calc IOPS";
        } else {
            std::cout << "Average IOPS = " << std:: fixed << totalOps / time_sec << std::endl;
            GLOGNOTIFY << "Average IOPS = " << std::fixed << totalOps / time_sec;
        }

        // Clean up the threads
        for (unsigned x = 0; x < concurrency; ++x) {
            std::thread* th = threads_[x];
            delete th;
            threads_[x] = NULL;
        }

        return 0;
    }

  private:
    /// performance test setup
    fds_uint32_t concurrency;
    /// total number of ops to execute
    fds_uint32_t totalOps;
    /// blob size in bytes
    fds_uint32_t blobSize;
    /// profiling enabled
    fds_bool_t profile;
    /// profile file
    std::string profileFile;
    /// use thrift
    fds_bool_t thrift;

    /// threads for blocking AM
    std::vector<std::thread*> threads_;

    /// to count number of ops issued
    std::atomic<fds_uint32_t> opCount;
    /// to count number of ops completed
    std::atomic<fds_uint32_t> opsDone;

    /// total latency observed by each thread
    std::atomic<fds_uint64_t> avgLatencyTotal;

    /// Latency for async ops
    fds_uint64_t asyncStartNano;
    fds_uint64_t asyncStopNano;

    /// domain name
    boost::shared_ptr<std::string> domainName;
    boost::shared_ptr<std::string> volumeName;

    /// Async response handler
    boost::shared_ptr<AmLoadProc> responseApi;

    /// Cond vars to sync responses
    std::condition_variable done_cond;
    std::mutex done_mutex;

    /// Data API to AM
    boost::shared_ptr<apis::AsyncAmServiceRequestIf> asyncDataApi;

    /// Thrift client
    boost::shared_ptr<apis::AsyncAmServiceRequestClient> asyncThriftClient;
    /// Thrift IP
    std::string serverIp;
    /// Thrift port
    fds_uint32_t serverPort;
    /// Thrift response port
    fds_uint32_t responsePort;
    /// Thrift response server
    boost::shared_ptr<xdi_ats::TThreadedServer> ttServer;
    /// Response server thread
    boost::shared_ptr<boost::thread> listen_thread;

    /// Thrift server parameters
    boost::shared_ptr<xdi_att::TServerTransport>  serverTransport;
    boost::shared_ptr<xdi_att::TTransportFactory> transportFactory;
    boost::shared_ptr<xdi_atp::TProtocolFactory>  protocolFactory;
    boost::shared_ptr<apis::AsyncAmServiceResponseProcessor> processor;
};

}  // namespace fds

using fds::AmLoadProc;
AmLoadProc::unique_ptr amLoad;

TEST(AccessMgr, updateBlobOnce) {
    GLOGDEBUG << "Testing updateBlobOnce";
    amLoad->runTask(AmLoadProc::PUT);
}

TEST(AccessMgr, getBlob) {
    GLOGDEBUG << "Testing getBlob";
    amLoad->runTask(AmLoadProc::GET);
}

TEST(AccessMgr, startBlobTx) {
    GLOGDEBUG << "Testing startBlobTx";
    amLoad->runTask(AmLoadProc::STARTTX);
}

TEST(AccessMgr, asyncStartBlobTx) {
    GLOGDEBUG << "Testing async startBlobTx";
    amLoad->runAsyncTask(AmLoadProc::STARTTX);
}

TEST(AccessMgr, asyncUpdateBlob) {
    GLOGDEBUG << "Testing async updateBlob";
    amLoad->runAsyncTask(AmLoadProc::PUT);
}

TEST(AccessMgr, asyncStatBlob) {
    GLOGDEBUG << "Testing async statBlob";
    amLoad->runAsyncTask(AmLoadProc::STAT);
}

TEST(AccessMgr, asyncGetBlob) {
    GLOGDEBUG << "Testing async getBlob";
    amLoad->runAsyncTask(AmLoadProc::GET);
}

TEST(AccessMgr, asyncVolumeContents) {
    GLOGDEBUG << "Testing async volumeContents";
    amLoad->runAsyncTask(AmLoadProc::LISTVOL);
}

TEST(AccessMgr, wr) {
    GLOGDEBUG << "Testing async write-read";
    amLoad->runAsyncTask(AmLoadProc::PUT);
    amLoad->runAsyncTask(AmLoadProc::GET);
}

TEST(AccessMgr, getWithMeta) {
    GLOGDEBUG << "Testing async getWithMeta";
    amLoad->runAsyncTask(AmLoadProc::GETWITHMETA);
}

int
main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    fds::Module *amVec[] = {
        nullptr
    };
    fds::AmProcessWrapper apw(argc, argv, "platform.conf", "fds.am.", amVec);
    apw.main();

    amLoad = AmLoadProc::unique_ptr(new AmLoadProc(argc, argv));
    amLoad->init();

    return RUN_ALL_TESTS();
}

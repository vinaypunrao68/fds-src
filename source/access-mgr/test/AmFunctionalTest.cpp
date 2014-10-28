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
#include <native_api.h>
#include <am-platform.h>
#include <net/net-service.h>
#include <AccessMgr.h>

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

class AmLoadProc : public AmAsyncResponseApi {
  public:
    AmLoadProc()
            : domainName(new std::string("Test Domain")),
              volumeName(new std::string("Test Volume")) {
        // register and populate volumes
        VolumeDesc volDesc(*volumeName, 5);
        volDesc.iops_min = 0;
        volDesc.iops_max = 0;
        volDesc.relativePrio = 1;
        fds_verify(am->registerVolume(volDesc) == ERR_OK);

        // hardcoding test config
        concurrency = 1;
        totalOps = 2000000;
        blobSize = 4096;
        opCount = ATOMIC_VAR_INIT(0);

        threads_.resize(concurrency);

        responseApi.reset(this, null_deleter());
        am->asyncDataApi->setResponseApi(responseApi);
    }
    virtual ~AmLoadProc() {
    }
    typedef std::unique_ptr<AmLoadProc> unique_ptr;

    enum TaskOps {
        PUT,
        GET,
        STARTTX
    };

    void startBlobTxResp(const Error &error,
                         boost::shared_ptr<apis::RequestId>& requestId,
                         boost::shared_ptr<apis::TxDescriptor>& txDesc) {
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
                am->asyncDataApi->updateBlobOnce(reqId,
                                                 domainName,
                                                 volumeName,
                                                 blobGen.blobName,
                                                 blobMode,
                                                 blobGen.blobData,
                                                 blobLength,
                                                 off,
                                                 meta);
            } else if (opType == STARTTX) {
                // Always use an empty request ID since we don't track
                boost::shared_ptr<apis::RequestId> reqId(
                    boost::make_shared<apis::RequestId>());
                am->asyncDataApi->startBlobTx(reqId,
                                              domainName,
                                              volumeName,
                                              blobGen.blobName,
                                              blobMode);
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
                    am->dataApi->updateBlobOnce(domainName, volumeName,
                                                blobGen.blobName, blobMode,
                                                blobGen.blobData, blobLength, off, meta);
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
        // ProfilerStart("/tmp/AM_output.prof");
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
        // ProfilerStop();
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
};

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

}  // namespace fds

int
main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    fds::Module *amVec[] = {
        nullptr
    };
    fds::AmProcessWrapper apw(argc, argv, "platform.conf", "fds.am.", amVec);
    apw.main();

    amLoad = AmLoadProc::unique_ptr(new AmLoadProc());

    return RUN_ALL_TESTS();
}

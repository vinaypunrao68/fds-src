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
#include <NbdOperations.h>
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
        am = AccessMgr::unique_ptr(new AccessMgr("AM Test Module", this));
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

class NbdOpsProc : public NbdOperationsResponseIface {
  public:
    NbdOpsProc(int argc, char **argv)
            : volumeName(new std::string("Test Volume")) {
        // register and populate volumes
        VolumeDesc volDesc(*volumeName, 5);
        volDesc.iops_min = 0;
        volDesc.iops_max = 0;
        volDesc.relativePrio = 1;
        fds_verify(am->registerVolume(volDesc) == ERR_OK);

        namespace po = boost::program_options;
        po::options_description desc("Nbd Operations functional test");
        desc.add_options()
                ("help,h", "Print this help message")
                ("threads,t",
                 po::value<fds_uint32_t>(&concurrency)->default_value(1),
                 "Number of threads doing async IO")
                ("load,l",
                 po::value<fds_uint32_t>(&outstandCount)->default_value(1),
                 "Number of requests outstanding from the tester")
                ("bsize,b",
                 po::value<fds_uint32_t>(&blobSize)->default_value(4096),
                 "Blob size in bytes")
                ("num-ops,n",
                 po::value<fds_uint32_t>(&totalOps)->default_value(5000),
                 "Number of operations");

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
    virtual ~NbdOpsProc() {
    }
    typedef std::unique_ptr<NbdOpsProc> unique_ptr;

    // implementation of NbdOperationsResponseIface
    void readWriteResp(const Error& error,
                       fds_int64_t handle,
                       NbdResponseVector* response) {
        fds_uint32_t cdone = atomic_fetch_add(&opsDone, (fds_uint32_t)1);
        GLOGDEBUG << "Read? " << response->isRead()
                  << " response for handle " << handle
                  << " totalOps " << totalOps
                  << " opsDone " << cdone;
        fds_verify(response->isReady());
        if (response->isRead()) {
            fds_uint32_t context = 0;
            boost::shared_ptr<std::string> buf = response->getNextReadBuffer(context);
            while (buf != NULL) {
                GLOGDEBUG << "Handle " << handle << "....Buffer # " << context;
                buf = response->getNextReadBuffer(context);
            }
        }
        // free response
        delete response;

        if (totalOps == (cdone + 1)) {
            asyncStopNano = util::getTimeStampNanos();
            done_cond.notify_all();
        }
    }

    void init() {
        // pass data API to Ndb Operations
        nbdOps.reset(new NbdOperations(am->asyncDataApi, this));
    }

    void resetCounters() {
      fds_uint32_t zeroCount = 0;
      atomic_store(&opCount, zeroCount);
      atomic_store(&opsDone, zeroCount);
    }

    enum TaskOps {
        PUT,
        GET
    };

    void task(int id, TaskOps opType) {
        fds_uint32_t ops = atomic_fetch_add(&opCount, (fds_uint32_t)1);
        GLOGDEBUG << "Starting thread " << id;
        fds_int64_t handle = id*totalOps;

        SequentialBlobDataGen blobGen(blobSize, id);
        while ((ops + 1) <= totalOps) {
            if (opType == PUT) {
                try {
                    // Make copy of data since function "takes" the shared_ptr
                    boost::shared_ptr<std::string> localData(
                        boost::make_shared<std::string>(*blobGen.blobData));
                    nbdOps->write(volumeName, 4096, localData, localData->length(), 0, ++handle);
                } catch(apis::ApiException fdsE) {
                    fds_panic("write failed");
                }
            } else if (opType == GET) {
                try {
                    nbdOps->read(volumeName, 4096, blobSize, 0, ++handle);
                } catch(apis::ApiException fdsE) {
                    fds_panic("read failed");
                }
            } else {
                fds_panic("Unknown op type");
            }
            ops = atomic_fetch_add(&opCount, (fds_uint32_t)1);
            blobGen.generateNext();
        }
    }

    virtual int runAsyncTask(TaskOps opType) {
        opCount = 0;
        opsDone = 0;
        asyncStartNano = util::getTimeStampNanos();
        // if (profile) ProfilerStart(profileFile.c_str());
        for (unsigned i = 0; i < concurrency; ++i) {
            std::thread* new_thread = new std::thread(&NbdOpsProc::task, this, i, opType);
            threads_[i] = new_thread;
        }

        // Wait for all threads
        for (unsigned x = 0; x < concurrency; ++x) {
            threads_[x]->join();
        }

        // Wait for responses
        std::unique_lock<std::mutex> lk(done_mutex);
        done_cond.wait_for(lk,
                           std::chrono::milliseconds(100000),
                           [this](){return totalOps == opsDone;});

        fds_uint64_t duration_nano = asyncStopNano - asyncStartNano;
        // if (profile) ProfilerStop();
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
    // NbdOperations to test
    NbdOperations::shared_ptr nbdOps;

    /// performance test setup
    fds_uint32_t concurrency;
    // number of requests outstanding from the tester
    // makes sense if async; if sync, concurrency
    fds_uint32_t outstandCount;
    // is the same as number of requests outstanding
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

    /// Cond vars to sync responses
    std::condition_variable done_cond;
    std::mutex done_mutex;
};

}  // namespace fds

using fds::NbdOpsProc;
NbdOpsProc::unique_ptr nbdOpsProc;

TEST(NbdOperations, read) {
    GLOGDEBUG << "TBD: Testing read";
    nbdOpsProc->runAsyncTask(NbdOpsProc::GET);
}

TEST(NbdOperations, write) {
    GLOGDEBUG << "TBD: Testing write";
    nbdOpsProc->resetCounters();
    nbdOpsProc->runAsyncTask(NbdOpsProc::PUT);
}

int
main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    fds::Module *amVec[] = {
        nullptr
    };
    fds::AmProcessWrapper apw(argc, argv, "platform.conf", "fds.am.", amVec);
    apw.main();

    nbdOpsProc = NbdOpsProc::unique_ptr(new NbdOpsProc(argc, argv));
    nbdOpsProc->init();

    return RUN_ALL_TESTS();
}

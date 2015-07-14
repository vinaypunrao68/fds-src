/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <string>
#include <vector>
#include <map>
#include <thread>
#include <condition_variable>
#include <util/fds_stat.h>
#include "connector/block/NbdOperations.h"
#include <AccessMgr.h>
#include <AmProcessor.h>

#include "boost/program_options.hpp"
#include <google/profiler.h>
#include <gtest/gtest.h>
#include <testlib/DataGen.hpp>
#include "fds_process.h"

namespace fds {

std::unique_ptr<AccessMgr> am;

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
                 po::value<fds_uint32_t>(&objSize)->default_value(4096),
                 "Object size in bytes")
                ("verify,v",
                 po::value<fds_bool_t>(&verifyData)->default_value(false),
                 "True to verify data")
                ("offset,o",
                 po::value<fds_uint64_t>(&firstOffset)->default_value(0),
                 "First offset on bytes")
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

    void terminate() override {
    }

    void attachResp(Error const& error,
                    boost::shared_ptr<VolumeDesc> const& volDesc) override {
        ASSERT_EQ(ERR_OK, error);
        objSize = volDesc->maxObjSizeInBytes;
    }

    // implementation of NbdOperationsResponseIface
    void readWriteResp(NbdResponseVector* response) override {
        fds_uint32_t cdone = atomic_fetch_add(&opsDone, (fds_uint32_t)1);
        GLOGDEBUG << "Read? " << response->isRead()
                  << " response for handle " << response->handle
                  << " totalOps " << totalOps
                  << " opsDone " << cdone;
        if (response->isRead()) {
            fds_uint32_t context = 0;
            boost::shared_ptr<std::string> dataWritten;
            if (verifyData) {
                fds_uint64_t off = response->getOffset();
                fds_mutex::scoped_lock l(verifyMutex);
                fds_verify(offData.count(off) > 0);
                dataWritten = offData[off];
            }
            size_t pos = 0;
            boost::shared_ptr<std::string> buf = response->getNextReadBuffer(context);
            while (buf != NULL) {
                GLOGDEBUG << "Handle " << response->handle << "....Buffer # " << context;
                if (verifyData) {
                    dataWritten->compare(pos, 4096, buf->c_str(), 4096);
                }
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
        nbdOps.reset(new NbdOperations(this));
        nbdOps->init(volumeName, am->getProcessor());
        am->getProcessor()->registerVolume(
            std::move(VolumeDesc(*volumeName, fds_volid_t(5), 0, 0, 1)));
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
        fds_uint64_t offset = firstOffset;

        SequentialBlobDataGen blobGen(objSize, id);
        while ((ops + 1) <= totalOps) {
            offset = ops * objSize + firstOffset;
            if (opType == PUT) {
                try {
                    // Make copy of data since function "takes" the shared_ptr
                    boost::shared_ptr<std::string> localData(
                        boost::make_shared<std::string>(*blobGen.blobData));
                    nbdOps->write(localData, localData->length(), offset, ++handle);
                    if (verifyData) {
                        fds_mutex::scoped_lock l(verifyMutex);
                        fds_verify(offData.count(offset) == 0);
                        offData[offset] = localData;
                    }
                } catch(fpi::ApiException fdsE) {
                    fds_panic("write failed");
                }
            } else if (opType == GET) {
                try {
                    nbdOps->read(objSize, offset, ++handle);
                } catch(fpi::ApiException fdsE) {
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
    fds_uint32_t objSize;
    /// verify that data we wrote is correct
    fds_bool_t verifyData;
    fds_uint64_t firstOffset;

    /// threads for blocking AM
    std::vector<std::thread*> threads_;

    /// to count number of ops issued
    std::atomic<fds_uint32_t> opCount;
    /// to count number of ops completed
    std::atomic<fds_uint32_t> opsDone;

    /// Latency for async ops
    fds_uint64_t asyncStartNano;
    fds_uint64_t asyncStopNano;

    /// domain name
    boost::shared_ptr<std::string> domainName;
    boost::shared_ptr<std::string> volumeName;

    /// Cond vars to sync responses
    std::condition_variable done_cond;
    std::mutex done_mutex;

    /// to verify data
    std::map<fds_uint64_t, boost::shared_ptr<std::string>> offData;
    fds_mutex verifyMutex;
};

}  // namespace fds

using fds::NbdOpsProc;
NbdOpsProc::unique_ptr nbdOpsProc;

TEST(NbdOperations, write) {
    GLOGDEBUG << "TBD: Testing write";
    nbdOpsProc->resetCounters();
    nbdOpsProc->runAsyncTask(NbdOpsProc::PUT);
}

TEST(NbdOperations, read) {
    GLOGDEBUG << "TBD: Testing read";
    nbdOpsProc->runAsyncTask(NbdOpsProc::GET);
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

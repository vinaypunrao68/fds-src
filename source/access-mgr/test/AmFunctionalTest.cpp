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

#include <gtest/gtest.h>
#include <testlib/DataGen.hpp>

namespace fds {

AccessMgr::unique_ptr am;

class AmLoadProc : public FdsProcess {
  public:
    AmLoadProc(int argc, char * argv[], const std::string & config,
               const std::string & basePath, Module * vec[])
            : FdsProcess(argc, argv, config, basePath, vec),
              domainName(new std::string("Test Domain")),
              volumeName(new std::string("Test Volume")) {
        am = AccessMgr::unique_ptr(new AccessMgr("AM Functional Test Module",
                                                 this));
        dataApi = am->dataApi;

        // hardcoding test config
        concurrency = 10;
        totalOps = 100;
        blobSize = 4096;
        putOp = true;
        opCount = ATOMIC_VAR_INIT(0);
    }
    virtual ~AmLoadProc() {
    }

    void task(int id) {
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
            if (putOp) {
                // do PUT
                am->dataApi->updateBlobOnce(domainName, volumeName,
                                            blobGen.blobName, blobMode,
                                            blobGen.blobData, blobLength, off, meta);
            } else {
                // do GET
            }
            ops = atomic_fetch_add(&opCount, (fds_uint32_t)1);
            blobGen.generateNext();
        }
    }

    virtual int run() override {
        // register and populate volumes
        VolumeDesc volDesc(*volumeName, 5);
        fds_verify(am->registerVolume(volDesc) == ERR_OK);

        if (!putOp) {
            // populate store (cache) before doing GET ops
        }

        fds_uint64_t start_nano = util::getTimeStampNanos();
        for (unsigned i = 0; i < concurrency; ++i) {
            std::thread* new_thread = new std::thread(&AmLoadProc::task, this, i);
            threads_.push_back(new_thread);
        }

        // Wait for all threads
        for (unsigned x = 0; x < concurrency; ++x) {
            threads_[x]->join();
        }

        for (unsigned x = 0; x < concurrency; ++x) {
            std::thread* th = threads_[x];
            delete th;
            threads_[x] = NULL;
        }
        fds_uint64_t duration_nano = util::getTimeStampNanos() - start_nano;
        double time_sec = duration_nano / 1000000000.0;
        if (time_sec < 10) {
            std::cout << "Experiment ran for too short time to calc IOPS" << std::endl;
            GLOGNOTIFY << "Experiment ran for too short time to calc IOPS";
        } else {
            std::cout << "Average IOPS = " << totalOps / time_sec << std::endl;
            GLOGNOTIFY << "Average IOPS = " << totalOps / time_sec;
        }

        return 0;
    }

  private:
    AmDataApi::shared_ptr dataApi;

    /// performance test setup
    fds_uint32_t concurrency;
    /// total number of ops to execute
    fds_uint32_t totalOps;
    /// blob size in bytes
    fds_uint32_t blobSize;
    fds_bool_t putOp;  // false == get

    /// threads for blocking AM
    std::vector<std::thread*> threads_;

    /// to count number of ops completed
    std::atomic<fds_uint32_t> opCount;

    /// domain name
    boost::shared_ptr<std::string> domainName;
    boost::shared_ptr<std::string> volumeName;
};

TEST(AccessMgr, statBlob) {
    GLOGDEBUG << "Testing stat blob";
    apis::BlobDescriptor blobDesc;
    boost::shared_ptr<std::string> domainName(
        boost::make_shared<std::string>("Test Domain"));
    boost::shared_ptr<std::string> volumeName(
        boost::make_shared<std::string>("Test Volume"));
    boost::shared_ptr<std::string> blobName(
        boost::make_shared<std::string>("Test Blob"));
    am->dataApi->statBlob(blobDesc, domainName, volumeName, blobName);
}

}  // namespace fds

int
main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    fds::Module *amVec[] = {
        nullptr
    };
    fds::AmLoadProc ap(argc, argv, "platform.conf", "fds.am.", amVec);
    ap.main();

    return RUN_ALL_TESTS();
}

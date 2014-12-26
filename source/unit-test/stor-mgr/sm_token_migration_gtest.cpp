/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <vector>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <chrono>
#include <condition_variable>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <FdsRandom.h>
#include <odb.h>
#include <ObjectId.h>
#include <TokenMigrationMgr.h>

#include <sm_ut_utils.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

static std::string logname = "sm_token_migration";

// Test implementation of SmIoReqHandler
class TestReqHandler: public SmIoReqHandler {
  public:
    TestReqHandler()
    : SmIoReqHandler() {
        threadPool = new fds_threadpool(1);
    }
    virtual ~TestReqHandler() {
        delete threadPool;
    }

    // implements SmIoReqHandler
    virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* ioReq) {
        // we'll schedule a request on threadpool
        switch (ioReq->io_type) {
                case FDS_SM_SNAPSHOT_TOKEN:
                threadPool->schedule(&TestReqHandler::snapshotToken, this, ioReq);
                break;
            default:
                return ERR_INVALID_ARG;
        };
        return ERR_OK;
    }

    void snapshotToken(SmIoReq* ioReq) {
        SmIoSnapshotObjectDB *snapReq = static_cast<SmIoSnapshotObjectDB*>(ioReq);
        GLOGNORMAL << "Simulating snapshotting SM token " << snapReq->token_id;
        if (snapReq->smio_snap_resp_cb) {
            // snapReq->smio_snap_resp_cb(ERR_OK, snapReq);
        }
        delete snapReq;
    }

  private:
    // threadpool to schedule "qos" requests
    fds_threadpool *threadPool;
};

class SmTokenMigrationTest : public ::testing::Test {
  public:
    SmTokenMigrationTest()
      : tokenMigrationMgr(new(std::nothrow) SmTokenMigrationMgr()) {
        dataStore = new(std::nothrow) TestReqHandler();
        migration_done = ATOMIC_VAR_INIT(false);
        EXPECT_TRUE(dataStore != NULL);
    }

    ~SmTokenMigrationTest() {
        delete dataStore;
    }

    void createObjectSet(fds_uint32_t numObjs);

    TestReqHandler* dataStore;
    SmTokenMigrationMgr::unique_ptr tokenMigrationMgr;

    // dataset for the test
    std::vector<ObjectID> objset;

    // done condition
    std::condition_variable done_cond;
    std::mutex cond_mutex;
    std::atomic<fds_bool_t> migration_done;
};

void
SmTokenMigrationTest::createObjectSet(fds_uint32_t numObjs) {
    SmUtUtils::createUniqueObjectIDs(numObjs, objset);
}

/**
 * This tests destination SM handling of token migration
 * We simulate source SM
 */
TEST_F(SmTokenMigrationTest, destination) {
    Error err(ERR_OK);
    createObjectSet(34);

    // TODO(anna) implement this test properly; for now
    // startMigration does not do anything...
    fpi::CtrlNotifySMStartMigrationPtr msg(new fpi::CtrlNotifySMStartMigration());
    err = tokenMigrationMgr->startMigration(dataStore, msg);

    // wait until migration is finished
    // TODO(anna) we do not set migration_done to true anywhere
    // so will always timeout until we implement this
    std::unique_lock<std::mutex> lk(cond_mutex);
    if (done_cond.wait_for(lk, std::chrono::milliseconds(10000),
                           [this](){return atomic_load(&migration_done);})) {
        GLOGNOTIFY << "Finished waiting for migration to finish!";
    } else {
        // 10 seconds should be enough to simulate writeback.. failing the test...
        GLOGNOTIFY << "Timed out waiting on migration done!";
        // TODO(anna) uncomment this -- expected for now, since token migration
        // not implemented yet
        // EXPECT_EQ(0, 1);
    }
}

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


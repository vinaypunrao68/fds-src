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
#include <TierMigration.h>

#include <sm_ut_utils.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

static std::string logname = "sm_tier_migration";
typedef std::function<void (std::vector<ObjectID>& migratedSet)> cbDoneType;

// Test implementation of SmIoReqHandler
class TestReqHandler: public SmIoReqHandler {
  public:
    explicit TestReqHandler(cbDoneType cb)
    : SmIoReqHandler(), doneCb(cb) {
        threadPool = new fds_threadpool(1);
        numObjsToMigrate = 0;
    }
    virtual ~TestReqHandler() {
        delete threadPool;
    }

    void setExpectedMigrateObjs(fds_uint32_t numObjs) {
        numObjsToMigrate = numObjs;
    }

    // implements SmIoReqHandler
    virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* ioReq) {
        // we'll schedule a request on threadpool
        switch (ioReq->io_type) {
            case FDS_SM_TIER_WRITEBACK_OBJECTS:
                threadPool->schedule(&TestReqHandler::writebackObjects, this, ioReq);
                break;
            case FDS_SM_TIER_PROMOTE_OBJECTS:
                threadPool->schedule(&TestReqHandler::promoteObjects, this, ioReq);
                break;
            default:
                return ERR_INVALID_ARG;
        };
        return ERR_OK;
    }

    void writebackObjects(SmIoReq* ioReq) {
        SmIoMoveObjsToTier *moveReq = static_cast<SmIoMoveObjsToTier*>(ioReq);
        GLOGNORMAL << "Simulating write-back of " << (moveReq->oidList).size() << " objects";
        for (fds_uint32_t i = 0; i < (moveReq->oidList).size(); ++i) {
            const ObjectID& objId = (moveReq->oidList)[i];
            GLOGNORMAL << ".....write-back " << objId;
            migrated_objset.push_back(objId);
        }

        if (moveReq->moveObjsRespCb) {
            moveReq->moveObjsRespCb(ERR_OK, moveReq);
        }
        delete moveReq;

        // check if we are done
        GLOGNORMAL << "Migrated " << migrated_objset.size() << " objects so far"
                   << " , of total " << numObjsToMigrate;
        if (migrated_objset.size() == numObjsToMigrate) {
            GLOGNORMAL << "Finished writing objects to HDD";
            doneCb(migrated_objset);
        }
    }

    void promoteObjects(SmIoReq* ioReq) {
        SmIoMoveObjsToTier *moveReq = static_cast<SmIoMoveObjsToTier*>(ioReq);
        if (moveReq->moveObjsRespCb) {
            moveReq->moveObjsRespCb(ERR_OK, moveReq);
        }
        delete moveReq;
    }

  private:
    // threadpool to schedule "qos" requests
    fds_threadpool *threadPool;

    /// number of objects to migrate (write-back/promote)
    fds_uint32_t numObjsToMigrate;
    std::vector<ObjectID> migrated_objset;

    /// callback to notify we finished migrating numObjsToMigrate
    cbDoneType doneCb;
};

class SmTierMigrationTest : public ::testing::Test {
  public:
    SmTierMigrationTest() {
        dataStore = new(std::nothrow) TestReqHandler(std::bind(
            &SmTierMigrationTest::notifyMigrationDone, this, std::placeholders::_1));
        EXPECT_TRUE(dataStore != NULL);
        migrator = new(std::nothrow) SmTierMigration(dataStore);
        EXPECT_TRUE(migrator != NULL);
        threadPool = new(std::nothrow) fds_threadpool(5);
        EXPECT_TRUE(threadPool != NULL);
    }

    ~SmTierMigrationTest() {
        delete dataStore;
        delete migrator;
        delete threadPool;
    }

    virtual void SetUp() override;
    virtual void TearDown() override;

    void createObjectSet(fds_uint32_t numObjs);
    void notifyMigrationDone(std::vector<ObjectID>& migratedSet);

    TestReqHandler* dataStore;
    SmTierMigration* migrator;
    // threadpool to generate work
    fds_threadpool *threadPool;

    // dataset for the test
    std::vector<ObjectID> objset;

    // done condition
    std::condition_variable done_cond;
    std::mutex cond_mutex;
    std::atomic<fds_bool_t> migration_done;
};

void
SmTierMigrationTest::SetUp() {
}

void
SmTierMigrationTest::TearDown() {
}

void
SmTierMigrationTest::createObjectSet(fds_uint32_t numObjs) {
    SmUtUtils::createUniqueObjectIDs(numObjs, objset);
    dataStore->setExpectedMigrateObjs(numObjs);
}

void notifyFlashPut(const ObjectID& oid,
                    SmTierMigration* migrator) {
    Error err(ERR_OK);
    err = migrator->notifyHybridVolFlashPut(oid);
    EXPECT_TRUE(err.ok());
}

void
SmTierMigrationTest::notifyMigrationDone(std::vector<ObjectID>& migratedSet) {
    GLOGNOTIFY << "Migration finished";
    migration_done = true;

    // verify we migrated the objects we wanted to migrate
    for (std::vector<ObjectID>::const_iterator cit = objset.cbegin();
         cit != objset.cend();
         ++cit) {
        EXPECT_NE(std::find(objset.begin(), objset.end(), *cit), objset.end());
    }

    // notify the test that is waiting for migration to finish
    done_cond.notify_all();
}

/**
 * This test simulates writing objects to SSD
 * and notifying SmTierMigration for write-back
 * Objects are "written" with multiple threads;
 * Test verifies that all objects are written back to HDD
 */
TEST_F(SmTierMigrationTest, writeback) {
    Error err(ERR_OK);
    createObjectSet(23);
    for (std::vector<ObjectID>::const_iterator cit = objset.cbegin();
         cit != objset.cend();
         ++cit) {
        threadPool->schedule(notifyFlashPut, *cit, migrator);
    }

    // wait until write-back is finished
    std::unique_lock<std::mutex> lk(cond_mutex);
    if (done_cond.wait_for(lk, std::chrono::milliseconds(10000),
                           [this](){return atomic_load(&migration_done);})) {
        GLOGNOTIFY << "Finished waiting for write-back to finish!";
    } else {
        // 10 seconds should be enough to simulate writeback.. failing the test...
        GLOGNOTIFY << "Timed out waiting on write-back done!";
        EXPECT_EQ(0, 1);
    }
}

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


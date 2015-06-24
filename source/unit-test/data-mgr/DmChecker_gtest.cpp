/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <cstdlib>
#include <fds_process.h>
#include <dm-vol-cat/DmPersistVolDB.h>
#include <DmChecker.h>

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>

using ::testing::AtLeast;
using ::testing::Return;

/* Fixture that
 * 1. Simulates two DMs hosting one volume with id 1
 * 2. Create two dm catalog dbs in setup and closes them during tear down
 */
struct DMCheckerFixture : fds::DMCheckerEnv, BaseTestFixture {
    void SetUp() override {
        db1Path = "/tmp/testdb1";
        db2Path = "/tmp/testdb2";
        /* Remove any existing dbs */
        auto res = std::system((std::string("rm -rf ") + db1Path).c_str());
        res = std::system((std::string("rm -rf ") + db2Path).c_str());

        /* Create new ones.  One volume db for each DM */
        leveldb::Options options;
        options.create_if_missing = true;
        auto status = leveldb::DB::Open(options, db1Path, &db1);
        ASSERT_TRUE(status.ok());
        status = leveldb::DB::Open(options, db2Path, &db2);
        ASSERT_TRUE(status.ok());

    }
    void closeDbs() {
        if (db1) {
            delete db1;
            db1 = nullptr;
        }
        if (db2) {
            delete db2;
            db2 = nullptr;
        }
    }
    void TearDown() override {
        closeDbs();
    }
    void putBlob(leveldb::DB* db, const BlobObjKey &key, const std::string &value) {
        Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));
        db->Put(leveldb::WriteOptions(), keyRec, Record(value));
    }
    std::list<fds_volid_t> getVolumeIds() const override {
        return {fds_volid_t(1)};
    }
    uint32_t getReplicaCount(const fds_volid_t &volId) const override {
        fds_verify(volId.get() == 1);
        return 2;
    }
    std::string getCatalogPath(const fds_volid_t &volId,
                               const uint32_t &replicaIdx) const override {
        fds_verify(volId.get() == 1);
        if (replicaIdx == 0) {
            return db1Path;
        }
        return db2Path;
    }

    std::string db1Path;
    std::string db2Path;
    leveldb::DB* db1;
    leveldb::DB* db2;
};

TEST_F(DMCheckerFixture, nomismatches) {
    putBlob(db1, BlobObjKey(1,1), "val1");
    putBlob(db1, BlobObjKey(1,2), "val2");

    putBlob(db2, BlobObjKey(1,1), "val1");
    putBlob(db2, BlobObjKey(1,2), "val2");

    closeDbs();

    fds::DMChecker checker(this);
    auto mismatches = checker.run();
    ASSERT_EQ(mismatches, 0);
}

TEST_F(DMCheckerFixture, mismatches) {
    putBlob(db1, BlobObjKey(1,1), "val1");
    putBlob(db1, BlobObjKey(1,2), "val2");

    putBlob(db2, BlobObjKey(1,1), "val1");

    closeDbs();

    fds::DMChecker checker(this);
    auto mismatches = checker.run();
    ASSERT_EQ(mismatches, 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
    return 0;
}

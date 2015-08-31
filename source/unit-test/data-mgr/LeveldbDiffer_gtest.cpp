/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <cstdlib>
#include <fds_process.h>
#include <checker/LeveldbDiffer.h>
#include <dm-vol-cat/DmPersistVolDB.h>

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {


}  // namespace fds

struct LevelDbDifferFixture :  BaseTestFixture {
    void SetUp() override {
        db1Path = "/tmp/testdb1";
        db2Path = "/tmp/testdb2";
        auto res = std::system((std::string("rm -rf ") + db1Path).c_str());
        res = std::system((std::string("rm -rf ") + db2Path).c_str());

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

    std::string db1Path;
    std::string db2Path;
    leveldb::DB* db1;
    leveldb::DB* db2;

};

TEST_F(LevelDbDifferFixture, keymatch) {
    db1->Put(leveldb::WriteOptions(), leveldb::Slice("key1"), leveldb::Slice("val1"));
    db1->Put(leveldb::WriteOptions(), leveldb::Slice("key2"), leveldb::Slice("val2"));
    db2->Put(leveldb::WriteOptions(), leveldb::Slice("key1"), leveldb::Slice("val1"));
    db2->Put(leveldb::WriteOptions(), leveldb::Slice("key2"), leveldb::Slice("val2"));
    closeDbs();

    LevelDbDiffAdapter adapter;
    std::vector<DiffResult> diffResults;
    LevelDbDiffer differ(db1Path, db2Path, &adapter);
    bool contDiff = differ.diff(16, diffResults);
    ASSERT_EQ(contDiff, false);
    ASSERT_EQ(diffResults.size(), 0);
}

TEST_F(LevelDbDifferFixture, deleted_key) {
    db1->Put(leveldb::WriteOptions(), leveldb::Slice("key1"), leveldb::Slice("val1"));
    db2->Put(leveldb::WriteOptions(), leveldb::Slice("key1"), leveldb::Slice("val1"));
    db2->Put(leveldb::WriteOptions(), leveldb::Slice("key2"), leveldb::Slice("val2"));
    db2->Put(leveldb::WriteOptions(), leveldb::Slice("key3"), leveldb::Slice("val3"));
    closeDbs();

    LevelDbDiffAdapter adapter;
    std::vector<DiffResult> diffResults;
    LevelDbDiffer differ(db1Path, db2Path, &adapter);
    bool contDiff = differ.diff(16, diffResults);
    ASSERT_EQ(contDiff, false);
    ASSERT_EQ(diffResults.size(), 2);
    ASSERT_EQ(diffResults[0].mismatchType, DiffResult::DELETED_KEY);
    ASSERT_EQ(diffResults[1].mismatchType, DiffResult::DELETED_KEY);
}

TEST_F(LevelDbDifferFixture, mixed_keys) {
    db1->Put(leveldb::WriteOptions(), leveldb::Slice("key1"), leveldb::Slice("val1"));
    db2->Put(leveldb::WriteOptions(), leveldb::Slice("key1"), leveldb::Slice("val1"));
    /* Extra key */
    db1->Put(leveldb::WriteOptions(), leveldb::Slice("key2"), leveldb::Slice("val2"));
    /* Delete key */
    db2->Put(leveldb::WriteOptions(), leveldb::Slice("key3"), leveldb::Slice("val3"));
    /* Value mismatch */
    db1->Put(leveldb::WriteOptions(), leveldb::Slice("key4"), leveldb::Slice("val4_1"));
    db2->Put(leveldb::WriteOptions(), leveldb::Slice("key4"), leveldb::Slice("val4_2"));
    closeDbs();

    LevelDbDiffAdapter adapter;
    std::vector<DiffResult> diffResults;
    LevelDbDiffer differ(db1Path, db2Path, &adapter);
    bool contDiff = differ.diff(16, diffResults);
    ASSERT_EQ(contDiff, false);
    ASSERT_EQ(diffResults.size(), 3);
    ASSERT_EQ(diffResults[0].mismatchType, DiffResult::ADDITIONAL_KEY);
    ASSERT_EQ(diffResults[1].mismatchType, DiffResult::DELETED_KEY);
    ASSERT_EQ(diffResults[2].mismatchType, DiffResult::VALUE_MISMATCH);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
    return 0;
}

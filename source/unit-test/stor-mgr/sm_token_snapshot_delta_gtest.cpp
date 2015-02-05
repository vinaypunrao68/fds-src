/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */
#include <iostream>
#include <gtest/gtest.h>
#include <leveldb/db.h>
#include <boost/filesystem.hpp>

#include "ObjMeta.h"
#include "MigrationTools.h"

constexpr char* test_db_path = "/tmp/sm_token_snapshot_delta_gtest_db";

struct DeltaTest : public ::testing::Test {
 protected:
  void SetUp() override {
      using namespace leveldb; // NOLINT
      Options options;
      options.create_if_missing = true;
      writeOption.sync = true;
      leveldb::DB* _ldb;
      ASSERT_TRUE(DB::Open(options, test_db_path, &_ldb).ok());
      ldb.reset(_ldb);

      using namespace fds; // NOLINT
      ObjectBuf buf;
      ObjMetaData omd;
      omd.serializeTo(buf);
      ldb->Put(writeOption, "test_30", *buf.data);
      ldb->Put(writeOption, "test_20", *buf.data);
      ldb->Put(writeOption, "test_10", *buf.data);
      ldb->Put(writeOption, "test_25", *buf.data);
      init_state = ldb->GetSnapshot();
  }

  void TearDown() override {
      if (init_state)
          ldb->ReleaseSnapshot(init_state);
      ldb.reset();
      boost::filesystem::remove_all(test_db_path);
  }
  std::unique_ptr<leveldb::DB> ldb;
  leveldb::Snapshot const* init_state;
  leveldb::WriteOptions writeOption;
};

TEST_F(DeltaTest, equivalentSnapshotsYieldsEmptyList) {
    using namespace fds; // NOLINT
    auto snap = ldb->GetSnapshot();

    metadata::metadata_diff_type diff;
    fds::metadata::diff(ldb.get(), init_state, snap, diff);
    ASSERT_TRUE(diff.empty());
    ldb->ReleaseSnapshot(snap);
}

TEST_F(DeltaTest, additionsAtFrontAreFound) {
    using namespace fds; // NOLINT
    ObjectBuf buf;
    ObjMetaData omd;
    omd.serializeTo(buf);

    ldb->Put(writeOption, "test_00", *buf.data);

    auto snap = ldb->GetSnapshot();

    metadata::metadata_diff_type diff;
    fds::metadata::diff(ldb.get(), init_state, snap, diff);
    ASSERT_EQ(diff.size(), 1);
    for (auto& p: diff) {
        ASSERT_FALSE(static_cast<bool>(p.first));
        ASSERT_TRUE(static_cast<bool>(p.second));
    }
    ldb->ReleaseSnapshot(snap);
}

TEST_F(DeltaTest, additionsAtBackAreFound) {
    using namespace fds; // NOLINT
    ObjectBuf buf;
    ObjMetaData omd;
    omd.serializeTo(buf);

    ldb->Put(writeOption, "test_31", *buf.data);
    ldb->Put(writeOption, "test_32", *buf.data);
    ldb->Put(writeOption, "test_50", *buf.data);

    auto snap = ldb->GetSnapshot();

    metadata::metadata_diff_type diff;
    fds::metadata::diff(ldb.get(), init_state, snap, diff);
    ASSERT_EQ(diff.size(), 3);
    for (auto& p: diff) {
        ASSERT_FALSE(static_cast<bool>(p.first));
        ASSERT_TRUE(static_cast<bool>(p.second));
    }
    ldb->ReleaseSnapshot(snap);
}

TEST_F(DeltaTest, additionsInMiddleAreFound) {
    using namespace fds; // NOLINT
    ObjectBuf buf;
    ObjMetaData omd;
    omd.serializeTo(buf);

    ldb->Put(writeOption, "test_23", *buf.data);
    ldb->Put(writeOption, "test_13", *buf.data);

    auto snap = ldb->GetSnapshot();

    metadata::metadata_diff_type diff;
    fds::metadata::diff(ldb.get(), init_state, snap, diff);
    ASSERT_EQ(diff.size(), 2);
    for (auto& p: diff) {
        ASSERT_FALSE(static_cast<bool>(p.first));
        ASSERT_TRUE(static_cast<bool>(p.second));
    }
    ldb->ReleaseSnapshot(snap);
}

TEST_F(DeltaTest, additionsScatteredAreFound) {
    using namespace fds; // NOLINT
    ObjectBuf buf;
    ObjMetaData omd;
    omd.serializeTo(buf);

    ldb->Put(writeOption, "test_00", *buf.data);
    ldb->Put(writeOption, "test_13", *buf.data);
    ldb->Put(writeOption, "test_23", *buf.data);
    ldb->Put(writeOption, "test_33", *buf.data);

    auto snap = ldb->GetSnapshot();

    metadata::metadata_diff_type diff;
    fds::metadata::diff(ldb.get(), init_state, snap, diff);
    ASSERT_EQ(diff.size(), 4);
    for (auto& p: diff) {
        ASSERT_FALSE(static_cast<bool>(p.first));
        ASSERT_TRUE(static_cast<bool>(p.second));
    }
    ldb->ReleaseSnapshot(snap);
}

TEST_F(DeltaTest, updatedElementCaptured) {
    using namespace fds; // NOLINT
    ObjectBuf buf;
    ObjMetaData data;
    data.incRefCnt();
    data.serializeTo(buf);

    ldb->Put(writeOption, "test_10", *buf.data);

    data.incRefCnt();
    data.serializeTo(buf);
    ldb->Put(writeOption, "test_20", *buf.data);

    auto snap = ldb->GetSnapshot();

    metadata::metadata_diff_type diff;
    fds::metadata::diff(ldb.get(), init_state, snap, diff);
    ASSERT_EQ(diff.size(), 2);
    for (auto& p: diff) {
        ASSERT_TRUE(static_cast<bool>(p.first));
        ASSERT_TRUE(static_cast<bool>(p.second));
    }
    ldb->ReleaseSnapshot(snap);
}

TEST_F(DeltaTest, mixedAdditionsAndUpdates) {
    using namespace fds; // NOLINT
    ObjectBuf buf;
    ObjMetaData data;
    data.incRefCnt();
    data.serializeTo(buf);

    ldb->Put(writeOption, "test_10", *buf.data);

    data.incRefCnt();
    data.serializeTo(buf);
    ldb->Put(writeOption, "test_40", *buf.data);

    auto snap = ldb->GetSnapshot();

    metadata::metadata_diff_type diff;
    fds::metadata::diff(ldb.get(), init_state, snap, diff);
    ASSERT_EQ(diff.size(), 2);
    ASSERT_TRUE(static_cast<bool>(diff.front().first));
    ASSERT_TRUE(static_cast<bool>(diff.front().second));
    ASSERT_FALSE(static_cast<bool>(diff.back().first));
    ASSERT_TRUE(static_cast<bool>(diff.back().second));
    ldb->ReleaseSnapshot(snap);
}

TEST_F(DeltaTest, canceledEventsYieldsEmptyList) {
    using namespace fds; // NOLINT

    ObjectBuf buf;
    ObjMetaData data;
    data.incRefCnt();   // refcnt == 1
    data.serializeTo(buf);

    ldb->Put(writeOption, "test_10", *buf.data);

    ObjectBuf buf1;
    ObjMetaData data1;  // refcnt == 0, same as init_state
    data1.serializeTo(buf1);

    ldb->Put(writeOption, "test_10", *buf1.data);

    auto snap = ldb->GetSnapshot();

    metadata::metadata_diff_type diff;
    fds::metadata::diff(ldb.get(), init_state, snap, diff);
    ASSERT_TRUE(diff.empty());
    ldb->ReleaseSnapshot(snap);
}


int main(int argc, char *argv[])
{
    using namespace boost::filesystem; // NOLINT
    remove_all(path(test_db_path));
    ::testing::InitGoogleTest(&argc, argv);
    auto res = RUN_ALL_TESTS();
    remove_all(path(test_db_path));
    return res;
}

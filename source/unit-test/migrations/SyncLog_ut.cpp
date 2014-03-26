/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0
#include <cstdlib>
#include <ctime>
#include <set>

#include <ObjMeta.h>
#include <StorMgr.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <TokenSyncSender.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds_test {
struct ObjMetaTsLess {
    bool operator() (const ObjMetaData& lhs, const ObjMetaData& rhs) const
    {
        return lhs.getModificationTs() < rhs.getModificationTs();
    }
};
/**
 * Tests SyncLog add
 */
TEST(SyncLog, add) {
    std::srand(std::time(0));

    std::set<ObjMetaData, ObjMetaTsLess> mdSet;
    TokenSyncLog syncLog("SyncLog_ut_ldb");

    /* Put some data into a set and SyncLog */
    for (uint32_t i = 0; i < 100; i++) {
        ObjMetaData m1;
        ObjectID objId("01234567890123456789");
        uint32_t obj_size = 10;
        fds_volid_t vol_id1 = 2;
        fds_volid_t vol_id2 = 3;
        obj_phy_loc_t phys_loc;
        phys_loc.obj_file_id = 5;
        phys_loc.obj_stor_offset = 100;
        phys_loc.obj_tier = diskio::DataTier::diskTier;

        m1.initialize(objId, obj_size);
        m1.getObjMap()->assoc_mod_time = std::rand();  // NOLINT
        m1.updateAssocEntry(objId, vol_id1);
        m1.updateAssocEntry(objId, vol_id2);
        m1.updatePhysLocation(&phys_loc);

        mdSet.insert(m1);
        syncLog.add(objId, m1);
    }

    EXPECT_TRUE(mdSet.size() == syncLog.size());

    leveldb::Iterator* syncItr = syncLog.iterator();
    syncItr->SeekToFirst();
    /* Iterate the set and sync log and make sure each entry matches */
    for (auto itr : mdSet) {
        EXPECT_TRUE(syncItr->Valid());

        ObjectID id;
        ObjMetaData entry;
        TokenSyncLog::parse_iterator(syncItr, id, entry);
        EXPECT_TRUE(entry == itr);

        syncItr->Next();
    }
}

}  // namespace fds_test

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

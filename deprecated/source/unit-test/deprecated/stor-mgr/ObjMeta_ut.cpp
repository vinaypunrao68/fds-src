/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <ObjMeta.h>
#include <StorMgr.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds_test {

/**
 * Tests read(), write() methods
 */
TEST(ObjMeta, read_write) {
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
    m1.updateAssocEntry(objId, vol_id1);
    m1.updateAssocEntry(objId, vol_id2);
    m1.updatePhysLocation(&phys_loc);

    ObjectBuf buf;
    EXPECT_TRUE(m1.serializeTo(buf));
    EXPECT_TRUE(m1.getEstimatedSize() == buf.size);

    ObjMetaData m2;
    EXPECT_TRUE(m2.deserializeFrom(buf));
    EXPECT_TRUE(m1 == m2);
}

}  // namespace fds_test

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

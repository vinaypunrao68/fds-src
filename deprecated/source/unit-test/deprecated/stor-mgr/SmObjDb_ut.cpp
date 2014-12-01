/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <SmObjDb.h>
#include <StorMgr.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>


#if 0
class Turtle {
 public:
    virtual ~Turtle() {}
    virtual void PenUp() = 0;
    virtual void PenDown() = 0;
    virtual void Forward(int distance) = 0;
    virtual void Turn(int degrees) = 0;
    virtual void GoTo(int x, int y) = 0;
    virtual int GetX() const = 0;
    virtual int GetY() const = 0;
};

class MockTurtle : public Turtle {
 public:
    MOCK_METHOD0(PenUp, void());
    MOCK_METHOD0(PenDown, void());
    MOCK_METHOD1(Forward, void(int distance));
    MOCK_METHOD1(Turn, void(int degrees));
    MOCK_METHOD2(GoTo, void(int x, int y));
    MOCK_CONST_METHOD0(GetX, int());
    MOCK_CONST_METHOD0(GetY, int());
};

TEST(PainterTest, CanDrawSomething) {
    MockTurtle turtle;                          // #2
    EXPECT_CALL(turtle, PenDown())              // #3
    .Times(AtLeast(1));

    turtle.PenDown();

    /*  Painter painter(&turtle);                   // #4

  EXPECT_TRUE(painter.DrawCircle(0, 0, 10));*/
}                                             // #5
#endif

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds_test {

class MockObjectStore : public ObjectStorMgr {
 public:
    MockObjectStore() {}
    MOCK_METHOD1(getTokenId, fds_token_id(const ObjectID& objId));
    MOCK_METHOD1(isTokenInSyncMode, bool(const fds_token_id &tokId));
};

TEST(OnDiskSmObjMetadata, marshall_unmarshall) {
    OnDiskSmObjMetadata diskMd1;
    meta_obj_map_t objMap;

    objMap.obj_refcnt = 1;
    objMap.obj_size = 16;
    objMap.obj_tier = DataTier::diskTier;

    diskMd1.meta_data.locMap.updateMap(objMap);

    ObjectBuf buf;
    buf.resize(diskMd1.marshalledSize());
    size_t sz1 = diskMd1.marshall(const_cast<char*>(buf.data.data()), buf.data.size());

    EXPECT_EQ(sz1, buf.data.size());

    OnDiskSmObjMetadata diskMd2;
    size_t sz2 = diskMd2.unmarshall(const_cast<char*>(buf.data.data()), buf.data.size());

    EXPECT_EQ(sz1, sz2);
    EXPECT_EQ(diskMd1.marshalledSize(), diskMd2.marshalledSize());
    EXPECT_TRUE(diskMd1 == diskMd2);
}

TEST(OnDiskSmObjMetadata, readLocations) {
    /* Setup mock */
    MockObjectStore mockObjStor;
    EXPECT_CALL(mockObjStor, getTokenId(::testing::_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(mockObjStor, isTokenInSyncMode(1))
        .WillRepeatedly(Return(false));

    fds_log log("SmObjDb_ut.log");
    SmObjDb db(&mockObjStor, "sm", &log);

    OnDiskSmObjMetadata diskMd1;
    meta_obj_map_t loc1;
    ObjectID objId("01234567890123456789");

    /* Write a location */
    loc1.obj_refcnt = 1;
    loc1.obj_size = 16;
    loc1.obj_tier = DataTier::diskTier;
    db.writeObjectLocation(objId, &loc1, false);

    /* Read the locations back */
    diskio::MetaObjMap objMaps;
    db.readObjectLocations(SmObjDb::NON_SYNC_MERGED, objId, objMaps);

    /* Compare */
    EXPECT_EQ(objMaps.getPodsP()->size(), (size_t)1);
}
}  // namespace fds_test

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

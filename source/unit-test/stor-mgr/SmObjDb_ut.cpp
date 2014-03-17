/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <SmObjDb.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>


using ::testing::AtLeast;                     // #1

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
    // EXPECT_EQ(diskMd1, diskMd2);
}

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

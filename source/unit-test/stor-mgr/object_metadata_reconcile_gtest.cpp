/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <pthread.h>
#include <random>

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fdsp/sm_types_types.h>
#include <fdsp/sm_api_types.h>
#include <fdsp_utils.h>

#include <fds_assert.h>
#include <ObjectId.h>

#include <ObjMeta.h>

namespace fds {

void
initMetaDataPropagate(fpi::CtrlObjectMetaDataPropagate &msg, uint64_t volId)
{
    // cleanup first
    msg.objectVolumeAssoc.clear();
    msg.objectData.clear();
    msg.objectSize = 0;

    // init
    fpi::MetaDataVolumeAssoc volAssoc;
    volAssoc.volumeAssoc = volId;
    volAssoc.volumeRefCnt = 1;
    msg.objectReconcileFlag = fpi::OBJ_METADATA_NO_RECONCILE;
    msg.objectVolumeAssoc.push_back(volAssoc);
    msg.objectRefCnt = 1;
    msg.objectCompressType = 0;
    msg.objectCompressLen = 0;
    msg.objectBlkLen = 4096;
    msg.objectFlags = 0;
    msg.objectExpireTime = 0;
}

    // objMetaDataPtr->setObjReconcileRequired();

TEST(ObjMetaData, test1)
{
    // Test case 1:
    // Create reconcile MetaData.  Do Put on the same object.
    // Expected results are:
    // 1) NO_RECONCILE flag
    // 2) # of vol assoc = 0;
    //
    std::default_random_engine eng((std::random_device())());
    std::uniform_int_distribution<int64_t> randGen(0, std::numeric_limits<int64_t>::max());
    int64_t ranNum = randGen(eng);

    std::vector<fds_volid_t> vols;

    std::string objData = std::to_string(ranNum);
    ObjectID oid = ObjIdGen::genObjectId(objData.c_str(), objData.size());

    ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());
    objMetaDataPtr->initializeDelReconcile(oid, invalid_vol_id);
    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());
    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

    fpi::CtrlObjectMetaDataPropagate msg;
    initMetaDataPropagate(msg, 1);

    objMetaDataPtr->reconcilePutObjMetaData(oid, objData.size(), invalid_vol_id);

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

    EXPECT_FALSE(objMetaDataPtr->isObjReconcileRequired());

    vols.clear();
    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(0, vols.size());
}

TEST(ObjMetaData, test2)
{
    // Test case 2:
    // create metadata with 2 reconcile events.
    // Do 2 PUTS on the object.
    // Expected results are:
    // 1) NO_RECONCILE flag
    // 2) # of vol assoc = 0;
    std::default_random_engine eng((std::random_device())());
    std::uniform_int_distribution<int64_t> randGen(0, std::numeric_limits<int64_t>::max());
    int64_t ranNum = randGen(eng);

    std::vector<fds_volid_t> vols;

    std::string objData = std::to_string(ranNum);
    ObjectID oid = ObjIdGen::genObjectId(objData.c_str(), objData.size());

    // first DELETE
    ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());
    objMetaDataPtr->initializeDelReconcile(oid, invalid_vol_id);
    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());

    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

    // second DELETE reconcile
    objMetaDataPtr->reconcileDelObjMetaData(oid, fds_volid_t(1), fds::util::getTimeStampMillis());

    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    vols.clear();
    objMetaDataPtr->getAssociatedVolumes(vols);

    EXPECT_EQ(2, vols.size());
    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

    // first PUT reconcile
    objMetaDataPtr->reconcilePutObjMetaData(oid, objData.size(), invalid_vol_id);

    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());

    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

    // pretend we wrote an object to the data store
    obj_phy_loc_t objPhyLoc;
    objPhyLoc.obj_tier == diskio::diskTier;
    objPhyLoc.obj_stor_loc_id = 1;
    objPhyLoc.obj_stor_offset = 0;
    objPhyLoc.obj_file_id = 2;
    objMetaDataPtr->updatePhysLocation(&objPhyLoc);

    // second PUT reconcile
    objMetaDataPtr->reconcilePutObjMetaData(oid, objData.size(), fds_volid_t(1));

    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    vols.clear();
    objMetaDataPtr->getAssociatedVolumes(vols);

    EXPECT_EQ(0, vols.size());
    EXPECT_TRUE(!objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE
}

TEST(ObjMetaData, test3)
{
    // Test case 3:
    // Simulate
    // 1) forward PUT ObjectID in volume 0.
    // 2) migrate PUT ObjectID in volume 1.
    //
    std::default_random_engine eng((std::random_device())());
    std::uniform_int_distribution<int64_t> randGen(0, std::numeric_limits<int64_t>::max());
    int64_t ranNum = randGen(eng);

    std::vector<fds_volid_t> vols;

    std::string objData = std::to_string(ranNum);
    ObjectID oid = ObjIdGen::genObjectId(objData.c_str(), objData.size());

    // Metadata
    ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());
    objMetaDataPtr->initialize(oid, objData.size());
    objMetaDataPtr->updateAssocEntry(oid, invalid_vol_id);
    EXPECT_EQ(1, objMetaDataPtr->getRefCnt());

    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());

    EXPECT_FALSE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

    fpi::CtrlObjectMetaDataPropagate msg;
    initMetaDataPropagate(msg, 1);

    objMetaDataPtr->reconcileDeltaObjMetaData(msg);
    EXPECT_EQ(2, objMetaDataPtr->getRefCnt());
    vols.clear();
    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(2, vols.size());
    EXPECT_FALSE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE
}

TEST(ObjMetaData, test4)
{
    // Test case 3:
    // Simulate
    // 1) forward PUT ObjectID in volume 1.
    // 2) migrate PUT ObjectID in volume 1.
    //
    std::default_random_engine eng((std::random_device())());
    std::uniform_int_distribution<int64_t> randGen(0, std::numeric_limits<int64_t>::max());
    int64_t ranNum = randGen(eng);

    std::vector<fds_volid_t> vols;

    std::string objData = std::to_string(ranNum);
    ObjectID oid = ObjIdGen::genObjectId(objData.c_str(), objData.size());

    // Metadata
    ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());
    objMetaDataPtr->initialize(oid, objData.size());
    objMetaDataPtr->updateAssocEntry(oid, fds_volid_t(1));
    EXPECT_EQ(1, objMetaDataPtr->getRefCnt());

    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());

    EXPECT_FALSE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

    fpi::CtrlObjectMetaDataPropagate msg;
    initMetaDataPropagate(msg, 1);

    objMetaDataPtr->reconcileDeltaObjMetaData(msg);
    EXPECT_EQ(2, objMetaDataPtr->getRefCnt());
    vols.clear();
    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());
    EXPECT_FALSE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE
}
TEST(ObjMetaData, test5)
{
    // Test case 3:
    // Simulate
    // 1) forward DELETE ObjectID in volume 1.
    // 2) migrate PUT ObjectID in volume 1.
    //
    std::default_random_engine eng((std::random_device())());
    std::uniform_int_distribution<int64_t> randGen(0, std::numeric_limits<int64_t>::max());
    int64_t ranNum = randGen(eng);

    std::vector<fds_volid_t> vols;

    std::string objData = std::to_string(ranNum);
    ObjectID oid = ObjIdGen::genObjectId(objData.c_str(), objData.size());

    // Metadata
    ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());
    objMetaDataPtr->initializeDelReconcile(oid, fds_volid_t(1));
    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());

    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

    fpi::CtrlObjectMetaDataPropagate msg;
    initMetaDataPropagate(msg, 1);

    objMetaDataPtr->reconcileDeltaObjMetaData(msg);
    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());
    vols.clear();
    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(0, vols.size());

    EXPECT_FALSE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

}

TEST(ObjMetaData, test6)
{
    // Test case 3:
    // Simulate
    // 1) forward DELETE ObjectID in volume 0.
    // 2) migrate PUT ObjectID in volume 1.
    //
    std::default_random_engine eng((std::random_device())());
    std::uniform_int_distribution<int64_t> randGen(0, std::numeric_limits<int64_t>::max());
    int64_t ranNum = randGen(eng);

    std::vector<fds_volid_t> vols;

    std::string objData = std::to_string(ranNum);
    ObjectID oid = ObjIdGen::genObjectId(objData.c_str(), objData.size());

    // Metadata
    ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());
    objMetaDataPtr->initializeDelReconcile(oid, invalid_vol_id);
    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());

    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE

    fpi::CtrlObjectMetaDataPropagate msg;
    initMetaDataPropagate(msg, 1);

    objMetaDataPtr->reconcileDeltaObjMetaData(msg);
    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());
    vols.clear();
    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(2, vols.size());

    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

#ifdef TEST_VERBOSE
    std::cout << objMetaDataPtr->logString();
#endif  // TEST_VERBOSE


}

}  // namespace fds

int
main(int argc, char *argv[])
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

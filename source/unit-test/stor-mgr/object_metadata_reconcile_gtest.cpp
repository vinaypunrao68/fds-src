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
initMetaDataPropagate(fpi::CtrlObjectMetaDataPropagate &msg)
{
    // cleanup first
    msg.objectVolumeAssoc.clear();
    msg.objectData.clear();
    msg.objectSize = 0;

    // init
    fpi::MetaDataVolumeAssoc volAssoc;
    volAssoc.volumeAssoc = 1;
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

    // objMetaDataPtr->updateAssocEntry(oid, volId);
    // objMetaDataPtr->setObjReconcileRequired();

TEST(ObjMetaData, test1)
{
    // Test case 1:
    // Create reconcile MetaData.  Do Put on the same object.
    // Expect results are:
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
    objMetaDataPtr->initializeDelReconcile(oid, 0);
    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());
    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

    std::cout << objMetaDataPtr->logString();

    fpi::CtrlObjectMetaDataPropagate msg;
    initMetaDataPropagate(msg);

    objMetaDataPtr->reconcilePutObjMetaData(oid, 0);

    std::cout << objMetaDataPtr->logString();

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
    std::default_random_engine eng((std::random_device())());
    std::uniform_int_distribution<int64_t> randGen(0, std::numeric_limits<int64_t>::max());
    int64_t ranNum = randGen(eng);

    std::vector<fds_volid_t> vols;

    std::string objData = std::to_string(ranNum);
    ObjectID oid = ObjIdGen::genObjectId(objData.c_str(), objData.size());

    // first DELETE
    ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());
    objMetaDataPtr->initializeDelReconcile(oid, 0);
    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());

    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

    std::cout << objMetaDataPtr->logString();

    // second DELETE reconcile
    objMetaDataPtr->reconcileDelObjMetaData(oid, 1, fds::util::getTimeStampMillis());

    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    vols.clear();
    objMetaDataPtr->getAssociatedVolumes(vols);

    EXPECT_EQ(2, vols.size());
    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

    std::cout << objMetaDataPtr->logString();

    // first PUT reconcile
    objMetaDataPtr->reconcilePutObjMetaData(oid, 0);

    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    objMetaDataPtr->getAssociatedVolumes(vols);
    EXPECT_EQ(1, vols.size());

    EXPECT_TRUE(objMetaDataPtr->isObjReconcileRequired());

    std::cout << objMetaDataPtr->logString();

    // second PUT reconcile
    objMetaDataPtr->reconcilePutObjMetaData(oid, 1);

    EXPECT_EQ(0, objMetaDataPtr->getRefCnt());

    vols.clear();
    objMetaDataPtr->getAssociatedVolumes(vols);

    EXPECT_EQ(0, vols.size());
    EXPECT_TRUE(!objMetaDataPtr->isObjReconcileRequired());

    std::cout << objMetaDataPtr->logString();

}

}  // namespace fds

int
main(int argc, char *argv[])
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

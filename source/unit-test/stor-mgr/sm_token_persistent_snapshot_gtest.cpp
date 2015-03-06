/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */
#include <iostream>
#include <gtest/gtest.h>
#include <leveldb/db.h>
#include <leveldb/copy_env.h>
#include <boost/filesystem.hpp>
#include <sm_ut_utils.h>
#include <odb.h>
#include <ObjectId.h>
#include <fds_types.h>
#include <fds_process.h>
#include "MigrationTools.h"
#include "ObjMeta.h"

std::string odb_path = "/tmp/sm_token_persistent_snapshot_gtest_odb";
std::string odb_snap_path = "/tmp/sm_token_persistent_snapshot_gtest_odbSnap";

struct PersistSnapTest : public ::testing::Test {
    protected:
    void SetUp() override {
        using namespace fds; //NOLINT
        Error err(ERR_OK);
        ObjectBuf buf;
        odb = SmUtUtils::createObjectDB(odb_path,odb_path);
        std::string obj;
        ObjectID oid;
        ObjMetaData omd;

        obj = "test_30";
        oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
        omd.serializeTo(buf);
        err = odb->Put(oid, buf);

        obj = "test_20";
        oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
        omd.serializeTo(buf);
        err = odb->Put(oid, buf);

        obj = "test_10";
        oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
        omd.serializeTo(buf);
        err = odb->Put(oid, buf);

        obj = "test_25";
        oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
        omd.serializeTo(buf);
        err = odb->Put(oid, buf);
        cenv = nullptr;
        FdsRootDir::fds_mkdir(odb_snap_path.c_str());
        err = odb->PersistentSnap(odb_snap_path + "/1", &cenv);
        GLOGNOTIFY << "Error is : "<<  err;
    }

    void TearDown() override {
        delete odb;
        boost::filesystem::remove_all(odb_path);
        boost::filesystem::remove_all(odb_snap_path);
    }

    osm::ObjectDB *odb;
    leveldb::CopyEnv *cenv;
};

TEST_F(PersistSnapTest, equivalentSnapshotsYieldsEmptyList) {
    using namespace fds; //NOLINT
    Error err(ERR_OK);
    metadata::metadata_diff_type diff;

    leveldb::DB* dbFromFirstSnap;
    leveldb::DB* dbFromSecondSnap;
    leveldb::Options options;
    options.create_if_missing = false;

    err = odb->PersistentSnap(odb_snap_path + "/2", &cenv);
    leveldb::Status status = leveldb::DB::Open(options, odb_snap_path + "/1", &dbFromFirstSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }

    status = leveldb::DB::Open(options, odb_snap_path + "/2", &dbFromSecondSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }

    fds::metadata::diff(dbFromFirstSnap,
                        dbFromSecondSnap,
                        diff);
    ASSERT_TRUE(diff.empty());
    delete dbFromFirstSnap;
    delete dbFromSecondSnap;
}

TEST_F(PersistSnapTest, additionsAtFrontAreFound) {
    using namespace fds; //NOLINT
    std::string obj;
    ObjectBuf buf;
    ObjMetaData omd;

    obj = "test_00";
    ObjectID oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
    omd.serializeTo(buf);
    odb->Put(oid, buf);

    metadata::metadata_diff_type diff;
    leveldb::DB* dbFromFirstSnap;
    leveldb::DB* dbFromSecondSnap;
    leveldb::Options options;

    odb->PersistentSnap(odb_snap_path + "/2", &cenv);
    leveldb::Status status = leveldb::DB::Open(options, odb_snap_path + "/1", &dbFromFirstSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }
    status = leveldb::DB::Open(options, odb_snap_path + "/2", &dbFromSecondSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }

    fds::metadata::diff(dbFromFirstSnap,
                        dbFromSecondSnap,
                        diff);
    ASSERT_EQ(diff.size(), 1);
    for (auto& p: diff) {
        ASSERT_FALSE(static_cast<bool>(p.first));
        ASSERT_TRUE(static_cast<bool>(p.second));
    }
    delete dbFromFirstSnap;
    delete dbFromSecondSnap;
}

TEST_F(PersistSnapTest, additionsAtBackAreFound) {
    using namespace fds; //NOLINT
    std::string obj;
    ObjectBuf buf;
    ObjMetaData omd;

    obj = "test_40";
    ObjectID oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
    omd.serializeTo(buf);
    odb->Put(oid, buf);

    metadata::metadata_diff_type diff;
    leveldb::DB* dbFromFirstSnap;
    leveldb::DB* dbFromSecondSnap;
    leveldb::Options options;

    odb->PersistentSnap(odb_snap_path + "/2", &cenv);
    leveldb::Status status = leveldb::DB::Open(options, odb_snap_path + "/1", &dbFromFirstSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }

    status = leveldb::DB::Open(options, odb_snap_path + "/2", &dbFromSecondSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }

    fds::metadata::diff(dbFromFirstSnap,
                        dbFromSecondSnap,
                        diff);
    ASSERT_EQ(diff.size(), 1);
    for (auto& p: diff) {
        ASSERT_FALSE(static_cast<bool>(p.first));
        ASSERT_TRUE(static_cast<bool>(p.second));
    }
    delete dbFromFirstSnap;
    delete dbFromSecondSnap;
}

TEST_F(PersistSnapTest, additionsScatteredAreFound) {
    using namespace fds; //NOLINT
    std::string obj;
    ObjectBuf buf;
    ObjMetaData omd;
    ObjectID oid;

    obj = "test_00";
    oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
    omd.serializeTo(buf);
    odb->Put(oid, buf);

    obj = "test_15";
    oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
    omd.serializeTo(buf);
    odb->Put(oid, buf);

    obj = "test_40";
    oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
    omd.serializeTo(buf);
    odb->Put(oid, buf);

    metadata::metadata_diff_type diff;
    leveldb::DB* dbFromFirstSnap;
    leveldb::DB* dbFromSecondSnap;
    leveldb::Options options;

    odb->PersistentSnap(odb_snap_path + "/2", &cenv);
    leveldb::Status status = leveldb::DB::Open(options, odb_snap_path + "/1", &dbFromFirstSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }

    status = leveldb::DB::Open(options, odb_snap_path + "/2", &dbFromSecondSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }

    fds::metadata::diff(dbFromFirstSnap,
                        dbFromSecondSnap,
                        diff);
    ASSERT_EQ(diff.size(), 3);
    for (auto& p: diff) {
        ASSERT_FALSE(static_cast<bool>(p.first));
        ASSERT_TRUE(static_cast<bool>(p.second));
    } 
    delete dbFromFirstSnap;
    delete dbFromSecondSnap;
}

TEST_F(PersistSnapTest, updatesReported) {
    using namespace fds; //NOLINT
    std::string obj;
    ObjectBuf buf;
    ObjMetaData omd;
    ObjectID oid;
    omd.incRefCnt();

    obj = "test_10";
    oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
    omd.serializeTo(buf);
    odb->Put(oid, buf);

    obj = "test_20";
    oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
    omd.serializeTo(buf);
    odb->Put(oid, buf);

    obj = "test_30";
    oid =  ObjIdGen::genObjectId(obj.c_str(), obj.size());
    omd.serializeTo(buf);
    odb->Put(oid, buf);

    metadata::metadata_diff_type diff;
    leveldb::DB* dbFromFirstSnap;
    leveldb::DB* dbFromSecondSnap;
    leveldb::Options options;

    odb->PersistentSnap(odb_snap_path + "/2", &cenv);
    leveldb::Status status = leveldb::DB::Open(options, odb_snap_path + "/1", &dbFromFirstSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }

    status = leveldb::DB::Open(options, odb_snap_path + "/2", &dbFromSecondSnap);
    if (!status.ok()) {
        GLOGNOTIFY <<"Could not open leveldb instance from snapshot. Error is " << status.ToString();
    }

    fds::metadata::diff(dbFromFirstSnap,
                        dbFromSecondSnap,
                        diff);
    ASSERT_EQ(diff.size(), 3);
    for (auto& p: diff) {
        ASSERT_TRUE(static_cast<bool>(p.first));
        ASSERT_TRUE(static_cast<bool>(p.second));
    } 
    delete dbFromFirstSnap;
    delete dbFromSecondSnap;
}

int main(int argc, char *argv[])
{
    using namespace boost::filesystem; // NOLINT
    remove_all(path(odb_path));
    remove_all(path(odb_snap_path));
    ::testing::InitGoogleTest(&argc, argv);
    auto res = RUN_ALL_TESTS();
    remove_all(path(odb_path));
    remove_all(path(odb_snap_path));
    return res;
}

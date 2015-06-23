/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <set>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <object-store/ObjectMetaDb.h>

#include <sm_ut_utils.h>
#include <sm_dataset.h>

using ::testing::AtLeast;
using ::testing::Return;

static const fds_uint32_t bitsPerDltToken = 8;
static const fds_volid_t volId(34);

namespace fds {

// this seem to be required to properly bring up globals like
// fds root, etc
class MetaDbUtProc : public FdsProcess {
  public:
    MetaDbUtProc(int argc, char * argv[], const std::string & config,
                 const std::string & basePath, Module * vec[]) {
        init(argc, argv, config, basePath, "sm_metadb_ut.log", vec);
    }

    virtual int run() override {
        return 0;
    }
};

class SmMetaDbTest : public ::testing::Test {
  public:
    SmMetaDbTest()
            : metaDb(new ObjectMetadataDb()),
              smDiskMap(new SmDiskMap("Test SM Disk Map")) {
        metaDb->setNumBitsPerToken(bitsPerDltToken);
    }

    virtual void SetUp() override;
    virtual void TearDown() override;

    void init();
    ObjMetaData::ptr allocObjMeta(const ObjectID& objId);

    ObjectMetadataDb* metaDb;
    SmDiskMap::ptr smDiskMap;
    /**
     * this is to remember whether we used fake disk map (created by us)
     * or actual disk map setup by the platform
     */
    fds_bool_t crtFakeDiskMap;
};

void
SmMetaDbTest::init() {
    // init SM disk map
    smDiskMap->mod_init(NULL);
    smDiskMap->loadPersistentState();
    DLT* dlt = new DLT(bitsPerDltToken, 1, 1, true);
    SmUtUtils::populateDlt(dlt, 1);
    GLOGDEBUG << "Using DLT: " << *dlt;
    smDiskMap->handleNewDlt(dlt);
}

void
SmMetaDbTest::SetUp() {
    SmUtUtils::cleanAllInDir(g_fdsprocess->proc_fdsroot()->dir_dev());
    SmUtUtils::setupDiskMap(g_fdsprocess->proc_fdsroot(), 12, 2);
    init();
}

void
SmMetaDbTest::TearDown() {
    // cleanup
    SmUtUtils::cleanAllInDir(g_fdsprocess->proc_fdsroot()->dir_dev());
    delete metaDb;
    metaDb = nullptr;
}

ObjMetaData::ptr
SmMetaDbTest::allocObjMeta(const ObjectID& objId) {
    ObjMetaData::ptr meta(new ObjMetaData());
    obj_phy_loc_t loc;
    loc.obj_stor_loc_id = 1;
    loc.obj_file_id = 2;
    loc.obj_stor_offset = 0;
    loc.obj_tier = diskio::DataTier::diskTier;

    meta->initialize(objId, 4096);
    meta->updateAssocEntry(objId, volId);
    meta->updatePhysLocation(&loc);

    return meta;
}

TEST_F(SmMetaDbTest, sm_token_ownership) {
    Error err(ERR_OK);
    fds_uint32_t objCount = 500;
    const FdsRootDir* rootDir = g_fdsprocess->proc_fdsroot();
    std::vector<ObjectID> objset;
    SmUtUtils::createUniqueObjectIDs(500, objset);

    // open levelDb files -- SM owns all SM tokens
    err = metaDb->openMetadataDb(smDiskMap);
    EXPECT_TRUE(err.ok());

    // all level files should exist
    fds_bool_t exists = false;
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
        exists = SmUtUtils::existsInSubdirs(rootDir->dir_dev(),
                                            std::string("SNodeObjIndex_") + std::to_string(tok),
                                            true);
        EXPECT_TRUE(exists);
    }

    // pretend we lost ownership of half of SM tokens
    SmTokenSet tokSet;
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; tok += 2) {
        tokSet.insert(tok);
    }
    err = metaDb->closeAndDeleteMetadataDbs(tokSet);
    EXPECT_TRUE(err.ok());

    // lost SM tokens levelDBs should not exist
    for (SmTokenSet::const_iterator cit = tokSet.cbegin();
         cit != tokSet.cend();
         ++cit) {
        exists = SmUtUtils::existsInSubdirs(rootDir->dir_dev(),
                                            std::string("SNodeObjIndex_") + std::to_string(*cit),
                                            true);
        EXPECT_FALSE(exists);
    }

    // puts for objects that SM owns should succeed, otherwise should fail
    for (fds_uint32_t i = 0; i < objset.size(); ++i) {
        ObjectID oid = objset[i];
        ObjMetaData::ptr meta = allocObjMeta(oid);
        err = metaDb->put(volId, oid, meta);
        fds_token_id smTokId = SmDiskMap::smTokenId(oid, bitsPerDltToken);
        LOGDEBUG << "Put for " << oid << " smToken " << smTokId
                 << " returned " << err;
        if (tokSet.count(smTokId) == 0) {
            // owned by SM
            EXPECT_TRUE(err.ok());
        } else {
            // not owned by SM
            EXPECT_TRUE(err == ERR_NOT_READY);
        }
    }

    // since we did not change smDiskMap, the following call
    // will create and open metadata files that were removed from the
    // close and delete call
    // we are pretending that DLT changed such that SM gained ownership
    // of all tokens again
    err = metaDb->openMetadataDb(smDiskMap);
    EXPECT_TRUE(err.ok());
}
}  // namespace fds

int main(int argc, char * argv[]) {
    fds::MetaDbUtProc objPeristProc(argc, argv, "platform.conf",
                                    "fds.sm.", NULL);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


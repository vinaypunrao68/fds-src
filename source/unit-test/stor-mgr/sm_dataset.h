/**
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_STOR_MGR_SM_DATASET_H_
#define SOURCE_UNIT_TEST_STOR_MGR_SM_DATASET_H_

#include <unistd.h>

#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <boost/shared_ptr.hpp>

#include <fds_config.hpp>
#include <ObjectId.h>
#include <fds_volume.h>
#include <dlt.h>
#include <FdsRandom.h>

namespace fds {
static const fds_uint32_t bitsPerDltToken = 8;

/**
 * Dataset that contains objects (data not stored but can be recreated)
 */
class TestDataset {
  public:
    TestDataset() {}
    ~TestDataset() {}

    /**
     * Data struct used to describe an object
     * Does not contain objectID, because we will use it
     * in ObjectID to TestObject map, so we will know ObjectID
     */
    struct TestObject {
        fds_uint32_t size_;
        // random numbers we will use to re-create object data
        // this is for allowing us not to store the whole object
        // data in the dataset, which we may want to make large                                                                                                                   
        fds_uint32_t rnum_;
        fds_uint32_t rnum2_;

        boost::shared_ptr<std::string> getObjectData() {
            boost::shared_ptr<std::string> objData(
                new std::string(std::to_string(rnum_) +
                                std::to_string(rnum2_)));
            objData->resize(size_, static_cast<char>(rnum_));
            return objData;
        }
        TestObject & operator =(const TestObject & rhs) {
            if (&rhs != this) {
                size_ = rhs.size_;
                rnum_ = rhs.rnum_;
                rnum2_ = rhs.rnum2_;
            }
            return *this;
        }
        fds_bool_t isValid(boost::shared_ptr<const std::string> data) {
            boost::shared_ptr<std::string> objData = getObjectData();
            return (*data == *objData);
        }
        TestObject() : size_(4096), rnum_(6732), rnum2_(84734) {}
        TestObject(fds_uint32_t sz,
                   fds_uint32_t rnum,
                   fds_uint32_t rnum2) :
        size_(sz), rnum_(rnum), rnum2_(rnum2) {
        }
        ~TestObject() {}
    };

    /**
     * dataset we will use for testing
     */
    std::vector<ObjectID> dataset_;
    /**
     * dataset object id -> TestObject
     * TestObject has enough info to test for correctness
     */
    std::unordered_map<ObjectID, TestObject, ObjectHash> dataset_map_;

  public:  // methods
    void generateDataset(fds_uint32_t dataset_size,
                         fds_uint32_t obj_size);
    void generateDatasetPerBucket(fds_uint32_t dataset_size,
                                  fds_uint32_t obj_size,
                                  fds_token_id bucket);
};

/* Volume descriptor and behavior used for testing */
class TestVolume {
  public:
    typedef enum {
        STORE_OP_GET,
        STORE_OP_PUT,
        STORE_OP_DELETE,
        STORE_OP_DUPLICATE
    } StoreOpType;

    TestVolume() : voldesc_("invalid", invalid_vol_id) {}
    TestVolume(fds_volid_t volume_id,
               std::string volname,
               FdsConfigAccessor &conf);
    TestVolume(fds_volid_t volume_id,
               std::string volname,
               fds_uint32_t concurrency,
               fds_uint32_t numOps,
               StoreOpType opType,
               fds_uint32_t datasetSize,
               fds_uint32_t objSize,
               fds_token_id bucket=0);
    ~TestVolume() {}

    typedef boost::shared_ptr<TestVolume> ptr;

    StoreOpType getOpTypeFromName(std::string& name);

    VolumeDesc voldesc_;
    fds_uint32_t concurrency_;
    fds_uint32_t num_ops_;
    StoreOpType op_type_;
    std::atomic<fds_uint32_t> dispatched_count;

    // dataset of this volume
    TestDataset testdata_;
};
typedef std::unordered_map<fds_volid_t, TestVolume::ptr> TestVolMap;

/**
 * A set of test volumes; each volume has its own dataset
 * and concurrency setting + qos and media policies
 */
class TestVolumeMap {
  public:
    TestVolumeMap() {}
    ~TestVolumeMap() {}

    void init(boost::shared_ptr<FdsConfig>& fdsconf,
              const std::string & basePath);

    TestVolMap volmap;
};

TestVolume::StoreOpType
TestVolume::getOpTypeFromName(std::string& name) {
    if (0 == name.compare(0, 3, "get")) return STORE_OP_GET;
    if (0 == name.compare(0, 3, "put")) return STORE_OP_PUT;
    if (0 == name.compare(0, 3, "dup")) return STORE_OP_DUPLICATE;
    if (0 == name.compare(0, 6, "delete")) return STORE_OP_DELETE;
    fds_verify(false);
    return STORE_OP_GET;
}

TestVolume::TestVolume(fds_volid_t volume_id,
                       std::string volname,
                       FdsConfigAccessor &conf)
        : voldesc_(volname, volume_id) {
    voldesc_.iops_assured = conf.get<fds_int64_t>("iops_min");
    voldesc_.iops_throttle = conf.get<fds_int64_t>("iops_max");
    voldesc_.relativePrio = conf.get<fds_uint32_t>("priority");
    voldesc_.mediaPolicy = fpi::FDSP_MEDIA_POLICY_HYBRID_PREFCAP;
    concurrency_ = conf.get<fds_uint32_t>("concurrency");
    num_ops_ = conf.get<fds_uint32_t>("num_ops");
    fds_uint32_t dataset_size = conf.get<fds_uint32_t>("dataset_size");
    fds_uint32_t obj_size = conf.get<fds_uint32_t>("object_size");
    std::string opstr = conf.get<std::string>("op_type");
    testdata_.generateDataset(dataset_size, obj_size);
    op_type_ = getOpTypeFromName(opstr);

    std::cout << "Volume " << volname << " will execute " << num_ops_
              << " operations of type "
              << op_type_ << " with concurrency " << concurrency_
              << ", dataset size " << dataset_size << " objects, "
              << " object size " << obj_size << " bytes" << std::endl;
    LOGDEBUG << "Volume " << volname << " will execute " << num_ops_
             << " operations of type "
             << op_type_ << " with concurrency " << concurrency_
             << ", dataset size " << dataset_size << " objects, "
             << " object size " << obj_size << " bytes" << std::endl;

    dispatched_count = ATOMIC_VAR_INIT(0);
}

TestVolume::TestVolume(fds_volid_t volume_id,
                       std::string volname,
                       fds_uint32_t concurrency,
                       fds_uint32_t numOps,
                       StoreOpType opType,
                       fds_uint32_t datasetSize,
                       fds_uint32_t objSize,
                       fds_token_id bucketId)
        : voldesc_(volname, volume_id) {
    voldesc_.iops_assured = 0;
    voldesc_.iops_throttle = 0;
    voldesc_.relativePrio = 1;
    voldesc_.mediaPolicy = fpi::FDSP_MEDIA_POLICY_HDD;
    concurrency_ = concurrency;
    num_ops_ = numOps;
    if (bucketId) {
        testdata_.generateDatasetPerBucket(datasetSize, objSize, bucketId);
    } else {
        testdata_.generateDataset(datasetSize, objSize);
    }
    op_type_ = opType;

    LOGDEBUG << "Volume " << volname << " will execute " << num_ops_
             << " operations of type "
             << op_type_ << " with concurrency " << concurrency_
             << ", dataset size " << datasetSize << " objects, "
             << " object size " << objSize << " bytes" << std::endl;

    dispatched_count = ATOMIC_VAR_INIT(0);
}

void TestVolumeMap::init(boost::shared_ptr<FdsConfig>& fdsconf,
                         const std::string & basePath) {
    FdsConfigAccessor conf(fdsconf, basePath);
    fds_uint32_t volidx = 0;
    while (1) {
        // search for volume0, volume1, etc keys in config
        // and populate volume map, break when we don't find
        // next volumeX key
        std::string volname = "volume" + std::to_string(volidx);
        if (!conf.exists(volname)) break;
        std::string vol_path = basePath + volname + ".";
        FdsConfigAccessor vol_conf(fdsconf, vol_path);
        fds_volid_t volId(volidx + 1);
        TestVolume::ptr volume(new TestVolume(volId, volname, vol_conf));
        fds_verify(volmap.count(volId) == 0);
        volmap[volId] = volume;

        // next volume
        ++volidx;
    }
}

void TestDataset::generateDataset(fds_uint32_t dataset_size,
                                  fds_uint32_t obj_size) {
    fds_uint64_t seed = RandNumGenerator::getRandSeed();
    RandNumGenerator rgen(seed);
    fds_uint32_t rnum = (fds_uint32_t)rgen.genNum();
    fds_uint32_t rnum2 = (fds_uint32_t)rgen.genNum();
    // clear existing map
    dataset_.clear();
    dataset_map_.clear();
    // populate dataset
    for (fds_uint32_t i = 0; i < dataset_size; ++i) {
        TestObject obj(obj_size, rnum, rnum2);
        boost::shared_ptr<std::string> objData = obj.getObjectData();
        ObjectID oid = ObjIdGen::genObjectId(objData->c_str(), objData->size());
        // we want every object ID in the dataset to be unique
        while (dataset_map_.count(oid) > 0) {
            rnum = (fds_uint32_t)rgen.genNum();
            obj.rnum_ = rnum;
            obj.rnum2_ = rnum2;
            objData = obj.getObjectData();
            oid = ObjIdGen::genObjectId(objData->c_str(), objData->size());
        }
        dataset_.push_back(oid);
        dataset_map_[oid] = obj;
        LOGDEBUG << "Dataset: " << oid << " size " << dataset_map_[oid].size_
                 << " rnum " << dataset_map_[oid].rnum_ << " (" << rnum
                 << "," << rnum2 << ")";
        rnum = (fds_uint32_t)rgen.genNum();
        rnum2 = (fds_uint32_t)rgen.genNum();
    }
}

void TestDataset::generateDatasetPerBucket(fds_uint32_t dataset_size,
                                           fds_uint32_t obj_size,
                                           fds_token_id bucket) {
    fds_uint64_t seed = RandNumGenerator::getRandSeed();
    RandNumGenerator rgen(seed);
    fds_uint32_t rnum = (fds_uint32_t)rgen.genNum();
    fds_uint32_t rnum2 = (fds_uint32_t)rgen.genNum();
    // clear existing map
    dataset_.clear();
    dataset_map_.clear();
    // populate dataset
    for (fds_uint32_t i = 0; i < dataset_size; ++i) {
        TestObject obj(obj_size, rnum, rnum2);
        boost::shared_ptr<std::string> objData = obj.getObjectData();
        ObjectID oid = ObjIdGen::genObjectId(objData->c_str(), objData->size());
        // we want every object ID in the dataset to be unique
        while ((dataset_map_.count(oid) > 0) ||
               (SmDiskMap::smTokenId(oid, bitsPerDltToken) != bucket)) {
            rnum = (fds_uint32_t)rgen.genNum();
            obj.rnum_ = rnum;
            obj.rnum2_ = rnum2;
            objData = obj.getObjectData();
            oid = ObjIdGen::genObjectId(objData->c_str(), objData->size());
        }
        dataset_.push_back(oid);
        dataset_map_[oid] = obj;
        LOGDEBUG << "Dataset: " << oid << " bucket " << SmDiskMap::smTokenId(oid, bitsPerDltToken)
                 << " size " << dataset_map_[oid].size_
                 << " rnum " << dataset_map_[oid].rnum_ << " (" << rnum
                 << "," << rnum2 << ")";
        rnum = (fds_uint32_t)rgen.genNum();
        rnum2 = (fds_uint32_t)rgen.genNum();
    }
}


}  // namespace fds

#endif

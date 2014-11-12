/**
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_STOR_MGR_SM_DATASET_H_
#define SOURCE_UNIT_TEST_STOR_MGR_SM_DATASET_H_

#include <unistd.h>

#include <vector>
#include <chrono>
#include <string>
#include <boost/shared_ptr.hpp>

#include <ObjectId.h>
#include <FdsRandom.h>

namespace fds {

/**
 * Dataset that contains unique objects (data not stored but can be recreated)
 */
class FdsObjectDataset {
  public:
    FdsObjectDataset() {}
    ~FdsObjectDataset() {}

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
};

void FdsObjectDataset::generateDataset(fds_uint32_t dataset_size,
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


}  // namespace fds

#endif

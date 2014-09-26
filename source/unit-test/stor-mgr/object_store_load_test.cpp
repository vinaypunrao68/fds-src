/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>

#include <vector>
#include <chrono>
#include <thread>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <fds_assert.h>
#include <fds_process.h>
#include <ObjectId.h>
#include <FdsRandom.h>
#include <StatsCollector.h>
#include <object-store/ObjectStore.h>

#include <sm_ut_utils.h>

namespace fds {

static StorMgrVolumeTable* volTbl;
static ObjectStore::unique_ptr objectStore;
static fds_volid_t singleVolId = 74;

/**
 * Load (performance) test for ObjectStore
 *
 * Unit test runs store_ut.num_threads number of threads and does
 * store_ut.num_ops operations of type store_ut.op_type (get/put/delete)
 * on ObjectStore
 */
class ObjectStoreLoadProc : public FdsProcess {
  public:
    ObjectStoreLoadProc(int argc, char * argv[], const std::string & config,
                        const std::string & basePath, Module * vec[]);
    virtual ~ObjectStoreLoadProc() {}

    virtual int run() override;

    virtual void task(int id);

  private:
    typedef enum {
        STORE_OP_GET,
        STORE_OP_PUT,
        STORE_OP_DELETE
    } StoreOpType;

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

    /* helper methods */
    StoreOpType getOpTypeFromName(std::string& name);
    Error populateStore();
    Error validateStore();

    // wrappers for get/put/delete
    Error get(fds_volid_t volId,
              const ObjectID& objId,
              boost::shared_ptr<const std::string>& objData);
    Error put(fds_volid_t volId,
              const ObjectID& objId,
              boost::shared_ptr<const std::string> objData);
    Error remove(fds_volid_t volId,
                 const ObjectID& objId);

  private:
    /**
     * Params from from config file
     */
    fds_uint32_t num_threads_;
    fds_uint32_t dataset_size_;
    fds_uint32_t num_ops_;
    StoreOpType op_type_;
    fds_uint32_t obj_size_;

    std::vector<std::thread*> threads_;
    std::atomic<fds_uint32_t> op_count;
    std::atomic<fds_bool_t> test_pass_;

    /* dataset we will use for testing
     * If operations are gets, we will first
     * populate DB with generated dataset, and then
     * do gets on DB; same for deletes
     */
    std::vector<ObjectID> dataset_;
    /* dataset object id -> TestObject
     * TestObject has enough info to test for correctness
     */
    std::unordered_map<ObjectID, TestObject, ObjectHash> dataset_map_;
};


ObjectStoreLoadProc::StoreOpType
ObjectStoreLoadProc::getOpTypeFromName(std::string& name) {
    if (0 == name.compare(0, 3, "get")) return STORE_OP_GET;
    if (0 == name.compare(0, 3, "put")) return STORE_OP_PUT;
    if (0 == name.compare(0, 6, "delete")) return STORE_OP_DELETE;
    fds_verify(false);
    return STORE_OP_GET;
}

ObjectStoreLoadProc::ObjectStoreLoadProc(int argc, char * argv[], const std::string & config,
                                         const std::string & basePath, Module * vec[])
        : FdsProcess(argc, argv, config, basePath, vec) {
    FdsConfigAccessor conf(get_conf_helper());
    std::string whatstr = conf.get<std::string>("test_what");
    std::string opstr = conf.get<std::string>("op_type");

    op_type_ = getOpTypeFromName(opstr);
    num_threads_ = conf.get<fds_uint32_t>("num_threads");
    num_ops_ = conf.get<fds_uint32_t>("num_ops");
    dataset_size_ = conf.get<fds_uint32_t>("dataset_size");
    obj_size_ = conf.get<fds_uint32_t>("object_size");

    std::cout << "Will execute " << num_ops_ << " operations of type "
              << op_type_ << " while running " << num_threads_ << " threads"
              << ", dataset size " << dataset_size_ << " objects" << std::endl;
    LOGNOTIFY << "Will execute " << num_ops_ << " operations of type "
              << op_type_ << " while running " << num_threads_ << " threads"
              << ", dataset size " << dataset_size_ << " objects";

    fds_uint32_t sm_count = 1;
    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    objectStore->handleNewDlt(dlt);
    VolumeDesc vdesc("objectstore_ut_volume", singleVolId);
    volTbl->registerVolume(vdesc);
    // TODO(anna) may want to set tier policy...

    // generate dataset of unique objIds
    fds_uint64_t seed = RandNumGenerator::getRandSeed();
    RandNumGenerator rgen(seed);
    fds_uint32_t rnum = (fds_uint32_t)rgen.genNum();
    fds_uint32_t rnum2 = (fds_uint32_t)rgen.genNum();
    for (fds_uint32_t i = 0; i < dataset_size_; ++i) {
        TestObject obj(obj_size_, rnum, rnum2);
        boost::shared_ptr<std::string> objData =
                obj.getObjectData();
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

    // initialize dynamicc counters
    op_count = ATOMIC_VAR_INIT(0);
    test_pass_ = ATOMIC_VAR_INIT(true);
    StatsCollector::singleton()->enableQosStats("ObjStoreLoadTest");

    // create dir where leveldb files will go
    proc_fdsroot()->fds_mkdir(proc_fdsroot()->dir_user_repo_objs().c_str());
}


Error
ObjectStoreLoadProc::get(fds_volid_t volId,
                         const ObjectID& objId,
                         boost::shared_ptr<const std::string>& objData) {
    Error err(ERR_OK);
    fds_uint64_t start_nano = util::getTimeStampNanos();
    if (objectStore) {
        objData = objectStore->getObject(volId, objId, err);
    } else {
        fds_panic("no known modules are initialized for get operation!");
    }
    if (!err.ok()) return err;

    // record stat
    fds_uint64_t lat_nano = util::getTimeStampNanos() - start_nano;
    StatsCollector::singleton()->recordEvent(volId,
                                             util::getTimeStampNanos(),
                                             STAT_AM_GET_OBJ,
                                             static_cast<double>(lat_nano) / 1000.0);
    return ERR_OK;
}

Error
ObjectStoreLoadProc::put(fds_volid_t volId,
                         const ObjectID& objId,
                         boost::shared_ptr<const std::string> objData) {
    Error err(ERR_OK);
    fds_uint64_t start_nano = util::getTimeStampNanos();
    if (objectStore) {
        err = objectStore->putObject(volId, objId, objData);
    } else {
        fds_panic("no known modules are initialized for put operation!");
    }
    if (!err.ok()) return err;

    // record stat
    fds_uint64_t lat_nano = util::getTimeStampNanos() - start_nano;
    StatsCollector::singleton()->recordEvent(volId,
                                             util::getTimeStampNanos(),
                                             STAT_AM_PUT_OBJ,
                                             static_cast<double>(lat_nano) / 1000.0);

    return ERR_OK;
}

Error ObjectStoreLoadProc::remove(fds_volid_t volId,
                                  const ObjectID& objId) {
    if (objectStore) {
        return objectStore->deleteObject(volId, objId);
    } else {
        fds_panic("no known modules are initialized for put operation!");
    }
    return ERR_OK;
}

Error ObjectStoreLoadProc::populateStore() {
    Error err(ERR_OK);
    // put all  objects in dataset to the object store
    for (fds_uint32_t i = 0; i < dataset_.size(); ++i) {
        ObjectID oid = dataset_[i];
        boost::shared_ptr<std::string> data =
                dataset_map_[oid].getObjectData();
        err = put(singleVolId, oid, data);
        if (!err.ok()) return err;
    }
    return err;
}

Error ObjectStoreLoadProc::validateStore() {
    Error err(ERR_OK);
    // validate all objects in the object store
    for (fds_uint32_t i = 0; i < dataset_.size(); ++i) {
        ObjectID oid = dataset_[i];
        boost::shared_ptr<const std::string> objData;
        err = get(singleVolId, oid, objData);
        if (!err.ok()) return err;

        if (!dataset_map_[oid].isValid(objData)) {
            LOGERROR << "DATA VERIFY FAILED for obj " << oid;
            return ERR_ONDISK_DATA_CORRUPT;
        } else {
            LOGDEBUG << "Verified object " << oid;
        }
    }
    return err;
}


int ObjectStoreLoadProc::run() {
    Error err(ERR_OK);
    int ret = 0;
    std::cout << "Starting test...";

    // if this is get or delete test, populate metastore
    // first with obj meta
    if (op_type_ != STORE_OP_PUT) {
        err = populateStore();
        if (!err.ok()) {
          std::cout << "Failed to populate meta store " << err << std::endl;
          LOGERROR << "Failed to populate meta store " << err;
          return -1;
        }
        std::cout << "Finished populating object store" << std::endl;
    }

    fds_uint64_t start_nano = util::getTimeStampNanos();
    for (unsigned i = 0; i < num_threads_; ++i) {
        std::thread* new_thread = new std::thread(&ObjectStoreLoadProc::task, this, i);
        threads_.push_back(new_thread);
    }

    // Wait for all threads
    for (unsigned x = 0; x < num_threads_; ++x) {
        threads_[x]->join();
    }

    for (unsigned x = 0; x < num_threads_; ++x) {
        std::thread* th = threads_[x];
        delete th;
        threads_[x] = NULL;
    }
    fds_uint64_t duration_nano = util::getTimeStampNanos() - start_nano;
    double time_sec = duration_nano / 1000000000.0;
    if (time_sec < 10) {
        std::cout << "Experiment ran for too short time to calc IOPS" << std::endl;
        LOGNOTIFY << "Experiment ran for too short time to calc IOPS";
    } else {
        std::cout << "Average IOPS = " << num_ops_ / time_sec << std::endl;
        LOGNOTIFY << "Average IOPS = " << num_ops_ / time_sec;
    }

    // read back and validate
    err = validateStore();
    if (!err.ok()) {
        std::cout << "Failed to validate data store " << err << std::endl;
        LOGERROR << "Failed to validate data store " << err;
        return -1;
    }

    fds_bool_t test_passed = test_pass_.load();
    if (test_passed) {
        std::cout << "Test PASSED" << std::endl;
    } else {
        std::cout << "Test FAILED" << std::endl;
        ret = -1;
    }

    return -1;
}

void ObjectStoreLoadProc::task(int id) {
    // task is executed here
    Error err(ERR_OK);
    fds_uint64_t seed = RandNumGenerator::getRandSeed();
    RandNumGenerator rgen(seed);
    fds_uint32_t ops = atomic_fetch_add(&op_count, (fds_uint32_t)1);

    std::cout << "Starting thread " << id
              << " current number of ops finished " << ops << std::endl;

    while ((ops + 1) <= num_ops_)
    {
        // do op
        fds_uint32_t index = ops % dataset_.size();
        ObjectID oid = dataset_[index];
        switch (op_type_) {
            case STORE_OP_PUT:
                {
                    boost::shared_ptr<std::string> data =
                            dataset_map_[oid].getObjectData();
                    err = put(singleVolId, oid, data);
                    break;
                }
            case STORE_OP_GET:
                {
                    boost::shared_ptr<const std::string> data;
                    err = get(singleVolId, oid, data);
                    break;
                }
            case STORE_OP_DELETE:
                {
                    err = remove(singleVolId, oid);
                    break;
                }
            default:
                fds_verify(false);   // new type added?
        }
        if (!err.ok()) {
            LOGERROR << "Failed operation " << err;
            break;
        }

        ops = atomic_fetch_add(&op_count, (fds_uint32_t)1);
    }

    if (!err.ok()) {
        test_pass_.store(false);
    }
}

}  // namespace fds

int main(int argc, char * argv[]) {
    volTbl = new StorMgrVolumeTable();

    // create data store
    objectStore = ObjectStore::unique_ptr(
            new ObjectStore("Load Test Object Store", volTbl));
    std::cout << "Will test ObjectStore" << std::endl;
    LOGNOTIFY << "Will test ObjectStore";

    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        objectStore.get(),
        nullptr
    };

    fds::ObjectStoreLoadProc p(argc, argv, "sm_ut.conf", "fds.store_load.", smVec);

    std::cout << "unit test " << __FILE__ << " started." << std::endl;
    p.main();
    std::cout << "unit test " << __FILE__ << " finished." << std::endl;
    return 0;
}


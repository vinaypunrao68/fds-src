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
#include <google/profiler.h>

#include <fds_assert.h>
#include <fds_process.h>
#include <ObjectId.h>
#include <FdsRandom.h>
#include <object-store/ObjectStore.h>

#include <sm_ut_utils.h>
#include <sm_dataset.h>

namespace fds {

static StorMgrVolumeTable* volTbl;
static ObjectStore::unique_ptr objectStore;
static fds_volid_t const singleVolId(74);

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
    /* helper methods */
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
    std::vector<std::thread*> threads_;
    std::atomic<fds_uint32_t> op_count;
    std::atomic<fds_bool_t> test_pass_;

    TestVolume::ptr volume_;

    // latency counters
    std::atomic<fds_uint64_t> put_lat_micro;
    std::atomic<fds_uint64_t> get_lat_micro;
};

ObjectStoreLoadProc::ObjectStoreLoadProc(int argc, char * argv[], const std::string & config,
                                         const std::string & basePath, Module * vec[])
        : FdsProcess(argc, argv, config, basePath, vec) {
    // initialize dynamic counters
    op_count = ATOMIC_VAR_INIT(0);
    test_pass_ = ATOMIC_VAR_INIT(true);
    put_lat_micro = ATOMIC_VAR_INIT(0);
    get_lat_micro = ATOMIC_VAR_INIT(0);
}


Error
ObjectStoreLoadProc::get(fds_volid_t volId,
                         const ObjectID& objId,
                         boost::shared_ptr<const std::string>& objData) {
    Error err(ERR_OK);
    fds_uint64_t start_nano = util::getTimeStampNanos();
    if (objectStore) {
        diskio::DataTier usedTier = diskio::maxTier;
        objData = objectStore->getObject(volId, objId, usedTier, err);
    } else {
        fds_panic("no known modules are initialized for get operation!");
    }
    if (!err.ok()) return err;

    // record stat
    fds_uint64_t lat_nano = util::getTimeStampNanos() - start_nano;
    double lat_micro = static_cast<double>(lat_nano) / 1000.0;
    fds_uint64_t lat = atomic_fetch_add(&get_lat_micro, (fds_uint64_t)lat_micro);
    return ERR_OK;
}

Error
ObjectStoreLoadProc::put(fds_volid_t volId,
                         const ObjectID& objId,
                         boost::shared_ptr<const std::string> objData) {
    Error err(ERR_OK);
    fds_uint64_t start_nano = util::getTimeStampNanos();
    if (objectStore) {
        diskio::DataTier tier;
        err = objectStore->putObject(volId, objId, objData, false, tier);
    } else {
        fds_panic("no known modules are initialized for put operation!");
    }
    if (!err.ok()) return err;

    // record stat
    fds_uint64_t lat_nano = util::getTimeStampNanos() - start_nano;
    double lat_micro = static_cast<double>(lat_nano) / 1000.0;
    fds_uint64_t lat = atomic_fetch_add(&put_lat_micro, (fds_uint64_t)lat_micro);
    return ERR_OK;
}

Error ObjectStoreLoadProc::remove(fds_volid_t volId,
                                  const ObjectID& objId) {
    if (objectStore) {
        return objectStore->deleteObject(volId, objId, false);
    } else {
        fds_panic("no known modules are initialized for put operation!");
    }
    return ERR_OK;
}

Error ObjectStoreLoadProc::populateStore() {
    Error err(ERR_OK);
    // put all  objects in dataset to the object store
    for (fds_uint32_t i = 0; i < (volume_->testdata_).dataset_.size(); ++i) {
        ObjectID oid = (volume_->testdata_).dataset_[i];
        boost::shared_ptr<std::string> data =
                (volume_->testdata_).dataset_map_[oid].getObjectData();
        err = put((volume_->voldesc_).volUUID, oid, data);
        if (!err.ok()) return err;
    }
    return err;
}

Error ObjectStoreLoadProc::validateStore() {
    Error err(ERR_OK);
    // validate all objects in the object store
    for (fds_uint32_t i = 0; i < (volume_->testdata_).dataset_.size(); ++i) {
        ObjectID oid = (volume_->testdata_).dataset_[i];
        boost::shared_ptr<const std::string> objData;
        err = get((volume_->voldesc_).volUUID, oid, objData);
        if (!err.ok()) return err;

        if (!(volume_->testdata_).dataset_map_[oid].isValid(objData)) {
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

    // clean data/metadata from /fds/dev/hdd*/
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    SmUtUtils::cleanFdsDev(dir);

    // add fake DLT
    fds_uint32_t sm_count = 1;
    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    objectStore->handleNewDlt(dlt);
    // register volume
    FdsConfigAccessor conf(get_conf_helper());
    volume_.reset(new TestVolume(singleVolId, "objectstore_ut_volume", conf));
    volTbl->registerVolume(volume_->voldesc_);

    // if this is get or delete test, populate metastore
    // first with obj meta
    if (volume_->op_type_ != TestVolume::STORE_OP_PUT) {
        err = populateStore();
        if (!err.ok()) {
          std::cout << "Failed to populate meta store " << err << std::endl;
          LOGERROR << "Failed to populate meta store " << err;
          return -1;
        }
        std::cout << "Finished populating object store" << std::endl;
    }

    fds_uint64_t start_nano = util::getTimeStampNanos();
    ProfilerStart("/tmp/SMObjStore_output.prof");
    for (unsigned i = 0; i < volume_->concurrency_; ++i) {
        std::thread* new_thread = new std::thread(&ObjectStoreLoadProc::task, this, i);
        threads_.push_back(new_thread);
    }

    // Wait for all threads
    for (unsigned x = 0; x < volume_->concurrency_; ++x) {
        threads_[x]->join();
    }
    ProfilerStop();

    fds_uint64_t duration_nano = util::getTimeStampNanos() - start_nano;
    double time_sec = duration_nano / 1000000000.0;
    fds_uint64_t lat_counter = 0;
    if (volume_->op_type_ == TestVolume::STORE_OP_GET) {
        lat_counter = atomic_load(&get_lat_micro);
    } else if (volume_->op_type_ == TestVolume::STORE_OP_PUT) {
        lat_counter = atomic_load(&put_lat_micro);
    }
    double total_ops = volume_->num_ops_;
    double ave_lat = 0;
    if (total_ops > 0) {
        ave_lat = static_cast<double>(lat_counter) / total_ops;
    }
    if (time_sec < 10) {
        std::cout << "Experiment ran for too short time to calc IOPS" << std::endl;
        LOGNOTIFY << "Experiment ran for too short time to calc IOPS";
    } else {
        std::cout << "Average IOPS = " << volume_->num_ops_ / time_sec
                  << "; Average latency " << ave_lat << " microsec" << std::endl;
        LOGNOTIFY << "Average IOPS = " << volume_->num_ops_ / time_sec
                  << "; Average latency " << ave_lat << " microsec";
    }

    for (unsigned x = 0; x < volume_->concurrency_; ++x) {
        std::thread* th = threads_[x];
        delete th;
        threads_[x] = NULL;
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
    fds_uint32_t ops = atomic_fetch_add(&op_count, (fds_uint32_t)1);

    std::cout << "Starting thread " << id
              << " current number of ops finished " << ops << std::endl;

    while ((ops + 1) <= volume_->num_ops_)
    {
        // do op
        fds_uint32_t index = ops % (volume_->testdata_).dataset_.size();
        ObjectID oid = (volume_->testdata_).dataset_[index];
        switch (volume_->op_type_) {
            case TestVolume::STORE_OP_PUT:
            case TestVolume::STORE_OP_DUPLICATE:
                {
                    boost::shared_ptr<std::string> data =
                            (volume_->testdata_).dataset_map_[oid].getObjectData();
                    err = put((volume_->voldesc_).volUUID, oid, data);
                    break;
                }
            case TestVolume::STORE_OP_GET:
                {
                    boost::shared_ptr<const std::string> data;
                    err = get((volume_->voldesc_).volUUID, oid, data);
                    break;
                }
            case TestVolume::STORE_OP_DELETE:
                {
                    err = remove((volume_->voldesc_).volUUID, oid);
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
                     new ObjectStore("Load Test Object Store", NULL, volTbl));
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


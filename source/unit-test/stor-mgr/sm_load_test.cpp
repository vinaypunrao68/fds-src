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
#include <StorMgr.h>

#include <sm_ut_utils.h>
#include <sm_dataset.h>

namespace fds {

static ObjectStorMgr* sm;

/**
 * Load (performance) test for StorMgr
 *
 * Unit test runs store_ut.num_threads number of threads and does
 * store_ut.num_ops operations of type store_ut.op_type (get/put/delete)
 * on ObjectStore
 */
class SmLoadProc : public FdsProcess {
  public:
    SmLoadProc(int argc, char * argv[], const std::string & config,
               const std::string & basePath, Module * vec[]);
    virtual ~SmLoadProc() {
        delete sm;
    }

    virtual int run() override;

    virtual void task(fds_volid_t volId);

  private:
    /* helper methods */
    Error populateStore(TestVolume::ptr& volume);
    Error validateStore(TestVolume::ptr& volume);

    // wrappers for get/put/delete
    Error get(fds_volid_t volId,
              const ObjectID& objId,
              boost::shared_ptr<const std::string>& objData);
    Error put(fds_volid_t volId,
              const ObjectID& objId,
              boost::shared_ptr<const std::string> objData);
    Error remove(fds_volid_t volId,
                 const ObjectID& objId);

  private:  // methods
    Error getSm(fds_volid_t volId,
                const ObjectID& objId,
                boost::shared_ptr<const std::string>& objData);
    Error putSm(fds_volid_t volId,
                const ObjectID& objId,
                boost::shared_ptr<const std::string> objData);
    Error removeSm(fds_volid_t volId,
                   const ObjectID& objId);
    Error regVolume(TestVolume::ptr& volume);

    // callbacks
    void putSmCb(const Error &err,
                 SmIoPutObjectReq* putReq);
    void getSmCb(const Error &err,
                 SmIoGetObjectReq *getReq);
    void removeSmCb(const Error &err,
                    SmIoDeleteObjectReq* delReq);

  private:
    /**
     * Params from from config file
     */
    fds_bool_t simulate;

    std::vector<std::thread*> threads_;
    std::atomic<fds_bool_t> test_pass_;

    // set of volumes, each volume has its own
    // policy, data set, concurrency
    TestVolumeMap volumes_;

    fds_bool_t validating_;
};

SmLoadProc::SmLoadProc(int argc, char * argv[], const std::string & config,
                       const std::string & basePath, Module * vec[])
        : FdsProcess(argc, argv, config, basePath, vec){
    sm->setModProvider(this);

    std::string utconfname = proc_fdsroot()->dir_fds_etc() + "sm_ut.conf";
    boost::shared_ptr<FdsConfig> fdsconf(new FdsConfig(utconfname, argc, argv));
    FdsConfigAccessor conf(fdsconf, "fds.sm_load.");
    volumes_.init(fdsconf, "fds.sm_load.");

    // initialize dynamic counters
    simulate = false;
    validating_ = false;
    test_pass_ = ATOMIC_VAR_INIT(true);
    StatsCollector::singleton()->enableQosStats("ObjStoreLoadTest");
}

Error
SmLoadProc::get(fds_volid_t volId,
                const ObjectID& objId,
                boost::shared_ptr<const std::string>& objData) {
    Error err(ERR_OK);
    fds_uint64_t start_nano = util::getTimeStampNanos();
    if (simulate) {
        boost::this_thread::sleep(boost::posix_time::microseconds(2000));
    } else {
        err = getSm(volId, objId, objData);
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
SmLoadProc::put(fds_volid_t volId,
                const ObjectID& objId,
                boost::shared_ptr<const std::string> objData) {
    Error err(ERR_OK);
    fds_uint64_t start_nano = util::getTimeStampNanos();
    if (simulate) {
        boost::this_thread::sleep(boost::posix_time::microseconds(2000));
    } else {
        err = putSm(volId, objId, objData);
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

Error SmLoadProc::remove(fds_volid_t volId,
                         const ObjectID& objId) {
    Error err(ERR_OK);
    if (simulate) {
        boost::this_thread::sleep(boost::posix_time::microseconds(2000));
    } else {
        err = removeSm(volId, objId);
    }
    return err;
}

Error SmLoadProc::populateStore(TestVolume::ptr& volume) {
    Error err(ERR_OK);
    // put all  objects in dataset to the object store
    fds_uint32_t dispatched = 0;
    for (fds_uint32_t i = 0;
         i < (volume->testdata_).dataset_.size();
         ++i) {
        while (1) {
            dispatched = atomic_load(&(volume->dispatched_count));
            if (dispatched < volume->concurrency_) {
                break;
            }
            boost::this_thread::sleep(boost::posix_time::microseconds(100));
        }

        ObjectID oid = (volume->testdata_).dataset_[i];
        boost::shared_ptr<std::string> data =
                (volume->testdata_).dataset_map_[oid].getObjectData();
        err = put((volume->voldesc_).volUUID, oid, data);
        if (!err.ok()) return err;
        dispatched = atomic_fetch_add(&(volume->dispatched_count), (fds_uint32_t)1);
    }

    // wait till all writes complete
    while (1) {
        dispatched = atomic_load(&(volume->dispatched_count));
        if (dispatched < 1) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::microseconds(100));
    }

    return err;
}

Error SmLoadProc::validateStore(TestVolume::ptr& volume) {
    Error err(ERR_OK);
    // validate all objects in the object store
    fds_uint32_t dispatched = 0;
    validating_ = true;
    for (fds_uint32_t i = 0;
         i < (volume->testdata_).dataset_.size();
         ++i) {
        while (1) {
            dispatched = atomic_load(&(volume->dispatched_count));
            if (dispatched < volume->concurrency_) {
                break;
            }
            boost::this_thread::sleep(boost::posix_time::microseconds(100));
        }

        ObjectID oid = (volume->testdata_).dataset_[i];
        boost::shared_ptr<const std::string> objData;
        err = get((volume->voldesc_).volUUID, oid, objData);
        if (!err.ok()) return err;
        dispatched = atomic_fetch_add(&(volume->dispatched_count), (fds_uint32_t)1);
    }

    // wait till all reads complete
    while (1) {
        dispatched = atomic_load(&(volume->dispatched_count));
        if (dispatched < 1) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::microseconds(100));
    }
    validating_ = false;
    return err;
}


int SmLoadProc::run() {
    Error err(ERR_OK);
    int ret = 0;
    std::cout << "Starting test..." << std::endl;

    // clean data/metadata from /fds/dev/hdd*/
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    SmUtUtils::cleanFdsDev(dir);

    // pass fake DLT to SM
    fds_uint32_t sm_count = 1;
    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    sm->objectStore->handleNewDlt(dlt);

    // register and populate volumes
    for (TestVolMap::iterator it = volumes_.volmap.begin();
         it != volumes_.volmap.end();
         ++it) {
        err = regVolume(it->second);
        fds_verify(err.ok());

        // if this is get or delete test, populate metastore
        // first with obj meta
        if ((it->second)->op_type_ != TestVolume::STORE_OP_PUT) {
            err = populateStore(it->second);
            if (!err.ok()) {
                std::cout << "Failed to populate meta store " << err << std::endl;
                LOGERROR << "Failed to populate meta store " << err;
                return -1;
            }
            std::cout << "Finished populating object store" << std::endl;
        }
    }

    fds_uint32_t num_volumes = volumes_.volmap.size();
    for (TestVolMap::iterator it = volumes_.volmap.begin();
         it != volumes_.volmap.end();
         ++it) {
        std::thread* new_thread = new std::thread(&SmLoadProc::task, this, it->first);
        threads_.push_back(new_thread);
    }

    // Wait for all threads
    for (unsigned x = 0; x < num_volumes; ++x) {
        threads_[x]->join();
    }

    for (unsigned x = 0; x < num_volumes; ++x) {
        std::thread* th = threads_[x];
        delete th;
        threads_[x] = NULL;
    }

    // read back and validate
    for (TestVolMap::iterator it = volumes_.volmap.begin();
         it != volumes_.volmap.end();
         ++it) {
        err = validateStore(it->second);
        if (!err.ok()) {
            std::cout << "Failed to validate data store " << err << std::endl;
            LOGERROR << "Failed to validate data store " << err;
            return -1;
        }
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

void SmLoadProc::task(fds_volid_t volId) {
    // task is executed here
    Error err(ERR_OK);
    fds_uint32_t dispatched = 0;
    fds_uint32_t ops = 0;
    TestVolume::ptr volume = volumes_.volmap[volId];

    std::cout << "Starting thread for volume " << volId
              << " current number of ops finished " << ops << std::endl;

    fds_uint64_t start_nano = util::getTimeStampNanos();
    while ((ops + 1) <= volume->num_ops_)
    {
        while (1) {
            dispatched = atomic_load(&(volume->dispatched_count));
            if (dispatched < volume->concurrency_) {
                break;
            }
            boost::this_thread::sleep(boost::posix_time::microseconds(100));
        }

        // do op
        fds_uint32_t index = ops % (volume->testdata_).dataset_.size();
        ObjectID oid = (volume->testdata_).dataset_[index];
        switch (volume->op_type_) {
            case TestVolume::STORE_OP_PUT:
            case TestVolume::STORE_OP_DUPLICATE:
                {
                    boost::shared_ptr<std::string> data =
                            (volume->testdata_).dataset_map_[oid].getObjectData();
                    err = put(volId, oid, data);
                    break;
                }
            case TestVolume::STORE_OP_GET:
                {
                    boost::shared_ptr<const std::string> data;
                    err = get(volId, oid, data);
                    break;
                }
            case TestVolume::STORE_OP_DELETE:
                {
                    err = remove(volId, oid);
                    break;
                }
            default:
                fds_verify(false);   // new type added?
        }
        if (!err.ok()) {
            LOGERROR << "Failed operation " << err;
            break;
        }

        dispatched = atomic_fetch_add(&(volume->dispatched_count), (fds_uint32_t)1);
        ops++;
    }

    if (!err.ok()) {
        test_pass_.store(false);
    }

    fds_uint64_t duration_nano = util::getTimeStampNanos() - start_nano;
    double time_sec = duration_nano / 1000000000.0;
    if (time_sec < 10) {
        std::cout << "Volume " << volId << " executed " << ops << " OPS, "
                  << "Experiment ran for too short time to calc IOPS" << std::endl;
        LOGNOTIFY << "Volume " << volId << " executed "<< ops << " OPS, "
                  << "Experiment ran for too short time to calc IOPS";
    } else {
        std::cout << "Volume " << volId << " executed " << ops << " OPS, "
                  << "Average IOPS = " << ops / time_sec << std::endl;
        LOGNOTIFY << "Volume " << volId << " executed "<< ops << " OPS, "
                  << "Average IOPS = " << ops / time_sec;
    }
}

Error
SmLoadProc::putSm(fds_volid_t volId,
                  const ObjectID& objId,
                  boost::shared_ptr<const std::string> objData) {
    Error err(ERR_OK);

    boost::shared_ptr<fpi::PutObjectMsg> putObjMsg(new fpi::PutObjectMsg());
    putObjMsg->volume_id = volId;
    putObjMsg->origin_timestamp = fds::util::getTimeStampMillis();
    putObjMsg->data_obj = *objData;
    putObjMsg->data_obj_len = (*objData).size();
    putObjMsg->data_obj_id.digest =
            std::string((const char *)objId.GetId(), (size_t)objId.GetLen());

    auto putReq = new SmIoPutObjectReq(putObjMsg);
    putReq->io_type = FDS_SM_PUT_OBJECT;
    putReq->setVolId(putObjMsg->volume_id);
    putReq->origin_timestamp = 0;
    putReq->setObjId(objId);
    putReq->data_obj = *objData;
    putReq->perfNameStr = "volume:" + std::to_string(putObjMsg->volume_id);
    putReq->opReqFailedPerfEventType = SM_PUT_OBJ_REQ_ERR;
    putReq->opReqLatencyCtx.type = SM_E2E_PUT_OBJ_REQ;
    putReq->opReqLatencyCtx.name = putReq->perfNameStr;
    putReq->opReqLatencyCtx.reset_volid(putObjMsg->volume_id);
    putReq->opLatencyCtx.type = SM_PUT_IO;
    putReq->opLatencyCtx.name = putReq->perfNameStr;
    putReq->opLatencyCtx.reset_volid(putObjMsg->volume_id);
    putReq->opQoSWaitCtx.type = SM_PUT_QOS_QUEUE_WAIT;
    putReq->opQoSWaitCtx.name = putReq->perfNameStr;
    putReq->opQoSWaitCtx.reset_volid(putObjMsg->volume_id);

    putReq->response_cb= std::bind(
        &SmLoadProc::putSmCb, this,
        std::placeholders::_1, std::placeholders::_2);

    err = sm->enqueueMsg(putReq->getVolId(), putReq);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoPutObjectReq to StorMgr.  Error: "
                 << err;
        putSmCb(err, putReq);
    }
    return err;
}

void
SmLoadProc::putSmCb(const Error &err,
                    SmIoPutObjectReq* putReq)
{
    fds_volid_t volId = putReq->getVolId();
    fds_verify(volumes_.volmap.count(volId) > 0);
    TestVolume::ptr vol = volumes_.volmap[volId];
    fds_uint32_t dispatched = atomic_fetch_sub(&(vol->dispatched_count), (fds_uint32_t)1);
    delete putReq;
}

Error
SmLoadProc::getSm(fds_volid_t volId,
                  const ObjectID& objId,
                  boost::shared_ptr<const std::string>& objData) {
    Error err(ERR_OK);

    boost::shared_ptr<fpi::GetObjectMsg> getObjMsg(new fpi::GetObjectMsg());
    getObjMsg->volume_id = volId;
    getObjMsg->data_obj_id.digest = std::string((const char *)objId.GetId(),
                                                (size_t)objId.GetLen());

    auto getReq = new SmIoGetObjectReq(getObjMsg);
    getReq->io_type = FDS_SM_GET_OBJECT;
    getReq->setVolId(getObjMsg->volume_id);
    getReq->setObjId(objId);
    getReq->obj_data.obj_id = getObjMsg->data_obj_id;
    getReq->perfNameStr = "volume:" + std::to_string(getObjMsg->volume_id);
    getReq->opReqFailedPerfEventType = SM_GET_OBJ_REQ_ERR;
    getReq->opReqLatencyCtx.type = SM_E2E_GET_OBJ_REQ;
    getReq->opReqLatencyCtx.name = getReq->perfNameStr;
    getReq->opReqLatencyCtx.reset_volid(getObjMsg->volume_id);
    getReq->opLatencyCtx.type = SM_GET_IO;
    getReq->opLatencyCtx.name = getReq->perfNameStr;
    getReq->opLatencyCtx.reset_volid(getObjMsg->volume_id);
    getReq->opQoSWaitCtx.type = SM_GET_QOS_QUEUE_WAIT;
    getReq->opQoSWaitCtx.name = getReq->perfNameStr;
    getReq->opQoSWaitCtx.reset_volid(getObjMsg->volume_id);

    getReq->response_cb = std::bind(
        &SmLoadProc::getSmCb, this,
        std::placeholders::_1, std::placeholders::_2);

    err = sm->enqueueMsg(getReq->getVolId(), getReq);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoReadObjectMetadata to StorMgr.  Error: "
                 << err;
        getSmCb(err, getReq);
    }
    return err;
}

void
SmLoadProc::getSmCb(const Error &err,
                    SmIoGetObjectReq *getReq)
{
    fds_volid_t volId = getReq->getVolId();
    fds_verify(volumes_.volmap.count(volId) > 0);
    TestVolume::ptr vol = volumes_.volmap[volId];
    if (validating_) {
        const ObjectID& oid = getReq->getObjId();
        boost::shared_ptr<std::string> data(new std::string(getReq->getObjectNetResp->data_obj));
        if (!(vol->testdata_).dataset_map_[oid].isValid(data)) {
            LOGERROR << "DATA VERIFY FAILED for obj " << oid;
            fds_verify(false);
        } else {
            LOGDEBUG << "Verified object " << oid;
        }
    }
    fds_uint32_t dispatched = atomic_fetch_sub(&(vol->dispatched_count), (fds_uint32_t)1);
    delete getReq;
}


Error
SmLoadProc::removeSm(fds_volid_t volId,
                     const ObjectID& objId) {
    Error err(ERR_OK);

    boost::shared_ptr<fpi::DeleteObjectMsg> expObjMsg(new fpi::DeleteObjectMsg());
    expObjMsg->volId = volId;
    auto delReq = new SmIoDeleteObjectReq();
    delReq->io_type = FDS_SM_DELETE_OBJECT;
    delReq->setVolId(expObjMsg->volId);
    delReq->origin_timestamp = expObjMsg->origin_timestamp;
    delReq->setObjId(ObjectID(expObjMsg->objId.digest));
    delReq->perfNameStr = "volume:" + std::to_string(expObjMsg->volId);
    delReq->opReqFailedPerfEventType = SM_DELETE_OBJ_REQ_ERR;
    delReq->opReqLatencyCtx.type = SM_E2E_DELETE_OBJ_REQ;
    delReq->opReqLatencyCtx.name = delReq->perfNameStr;
    delReq->opLatencyCtx.type = SM_DELETE_IO;
    delReq->opLatencyCtx.name = delReq->perfNameStr;
    delReq->opQoSWaitCtx.type = SM_DELETE_QOS_QUEUE_WAIT;
    delReq->opQoSWaitCtx.name = delReq->perfNameStr;

    delReq->response_cb = std::bind(
        &SmLoadProc::removeSmCb, this,
        std::placeholders::_1, std::placeholders::_2);

    err = sm->enqueueMsg(delReq->getVolId(), delReq);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoDeleteObjectReq to StorMgr.  Error: "
                 << err;
        removeSmCb(err, delReq);
    }

    return err;
}

void
SmLoadProc::removeSmCb(const Error &err,
                       SmIoDeleteObjectReq* delReq)
{
    fds_volid_t volId = delReq->getVolId();
    fds_verify(volumes_.volmap.count(volId) > 0);
    TestVolume::ptr vol = volumes_.volmap[volId];
    fds_uint32_t dispatched = atomic_fetch_sub(&(vol->dispatched_count), (fds_uint32_t)1);
    delete delReq;
}

Error
SmLoadProc::regVolume(TestVolume::ptr& volume) {
    FDSP_NotifyVolFlag vol_flag = FDSP_NOTIFY_VOL_NO_FLAG;
    fds_volid_t volumeId = volume->voldesc_.volUUID;
    Error err = sm->regVol(volume->voldesc_);
    if (err.ok()) {
        StorMgrVolume * vol = sm->getVol(volumeId);
        fds_assert(vol != NULL);

        fds_volid_t queueId = vol->getQueue()->getVolUuid();
        if (!sm->getQueue(queueId)) {
            err = sm->regVolQos(queueId, static_cast<FDS_VolumeQueue*>(
                vol->getQueue().get()));
        }

        if (err.ok()) {
            sm->createCache(volumeId, 1024 * 1024 * 8, 1024 * 1024 * 256);
        } else {
            sm->deregVol(volumeId);
        }
    }
    if (!err.ok()) {
        GLOGERROR << "Registration failed for vol id " << std::hex << volumeId
                  << std::dec << " error: " << err.GetErrstr();
    }
    return err;
}

}  // namespace fds

int main(int argc, char * argv[]) {
    sm = new fds::ObjectStorMgr(g_fdsprocess);
    objStorMgr  = sm;
    std::cout << "Will test SM" << std::endl;
    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        &fds::gl_objStats,
        sm,
        nullptr
    };
    fds::SmLoadProc p(argc, argv, "platform.conf", "fds.sm.", smVec);

    std::cout << "unit test " << __FILE__ << " started." << std::endl;
    p.main();

    delete sm;
    sm = NULL;
    std::cout << "unit test " << __FILE__ << " finished." << std::endl;
    return 0;
}


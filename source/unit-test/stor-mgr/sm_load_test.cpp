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
#include <boost/program_options.hpp>
#include <condition_variable>

#include <google/profiler.h>

#include <fds_assert.h>
#include <fds_process.h>
#include <ObjectId.h>
#include <FdsRandom.h>
#include <StorMgr.h>

#include <sm_ut_utils.h>
#include <sm_dataset.h>

namespace fds {

static ObjectStorMgr* storMgr;


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
        delete storMgr;
    }

    virtual int run() override;

    virtual void task(fds_volid_t volId);

  private:
    /* helper methods */
    Error populateStore(TestVolume::ptr& volume);
    Error validateStore(TestVolume::ptr& volume);

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
    std::vector<std::thread*> threads_;
    std::atomic<fds_bool_t> test_pass_;

    // config
    bool dropCaches;

    // enable google profiler
    bool enableProfiler;

    // set of volumes, each volume has its own
    // policy, data set, concurrency
    TestVolumeMap volumes_;

    fds_bool_t validating_;
    std::condition_variable done_cond;
    std::mutex done_mutex;

    // to ensure configure concurrency
    std::condition_variable concurrency_cond;
    std::mutex concurrency_mutex;

    // latency
    std::atomic<fds_uint64_t> put_lat_micro;
    std::atomic<fds_uint64_t> get_lat_micro;
};

SmLoadProc::SmLoadProc(int argc, char * argv[], const std::string & config,
                       const std::string & basePath, Module * vec[])
        : FdsProcess(argc, argv, config, basePath, vec){
    storMgr->setModProvider(this);

    std::string utconfname = proc_fdsroot()->dir_fds_etc() + "sm_ut.conf";
    boost::shared_ptr<FdsConfig> fdsconf(new FdsConfig(utconfname, argc, argv));
    FdsConfigAccessor conf(fdsconf, "fds.sm_load.");
    volumes_.init(fdsconf, "fds.sm_load.");

    namespace progOpt = boost::program_options;
    progOpt::options_description progDesc("SM Load Test Options");
    progDesc.add_options()
        ("help,h", "Help Message")
        ("drop-cache",
         progOpt::value<bool>(&dropCaches)->default_value(false),
         "Drop filesystem cache")
        ("enable-profiler",
         progOpt::value<bool>(&enableProfiler)->default_value(false),
         "Google profiler");

    progOpt::variables_map varMap;
    progOpt::parsed_options parsedOpt =
        progOpt::command_line_parser(argc, argv).options(progDesc).allow_unregistered().run();
    progOpt::store(parsedOpt, varMap);
    progOpt::notify(varMap);

    std::cout << "===== options list =====" << std::endl;
    std::cout << "     dropCaches=" << dropCaches << std::endl;
    std::cout << "     enableProfile=" << enableProfiler << std::endl;

    if (varMap.count("help")) {
        std::cout << progDesc << std::endl;
        exit(0);
    }

    std::cout << "dropCaches=" << dropCaches << std::endl;
    std::cout << "enableProfile=" << dropCaches << std::endl;


    // initialize dynamic counters
    // dropCaches = false;
    // enableProfiler = false;
    validating_ = false;
    test_pass_ = ATOMIC_VAR_INIT(true);
    put_lat_micro = ATOMIC_VAR_INIT(0);
    get_lat_micro = ATOMIC_VAR_INIT(0);
    StatsCollector::singleton()->enableQosStats("ObjStoreLoadTest");
}

Error SmLoadProc::populateStore(TestVolume::ptr& volume) {
    Error err(ERR_OK);
    // put all  objects in dataset to the object store
    fds_uint32_t dispatched = 0;
    for (fds_uint32_t i = 0;
         i < (volume->testdata_).dataset_.size();
         ++i) {
        if (atomic_load(&(volume->dispatched_count)) >= volume->concurrency_) {
            std::unique_lock<std::mutex> lk(concurrency_mutex);
            if (concurrency_cond.wait_for(lk, std::chrono::milliseconds(10000),
                [volume](){
                return (atomic_load(&(volume->dispatched_count)) < volume->concurrency_);})) {
                GLOGTRACE << "Will send IO: " << atomic_load(&(volume->dispatched_count));
            } else {
                GLOGWARN << "Timeout waiting for request to complete";
                return ERR_SVC_REQUEST_TIMEOUT;  // timeout should not happen :(
            }
        }

        ObjectID oid = (volume->testdata_).dataset_[i];
        boost::shared_ptr<std::string> data =
                (volume->testdata_).dataset_map_[oid].getObjectData();
        err = putSm((volume->voldesc_).volUUID, oid, data);
        if (!err.ok()) return err;
        dispatched = atomic_fetch_add(&(volume->dispatched_count), (fds_uint32_t)1);
    }

    // wait till all writes complete
    std::unique_lock<std::mutex> lk(done_mutex);
    if (done_cond.wait_for(lk, std::chrono::milliseconds(10000),
              [volume](){return (atomic_load(&(volume->dispatched_count)) < 1);})) {
        GLOGNOTIFY << "Finished populating store...";
    } else {
        GLOGWARN << "Timeout waiting for all reads to complete";
        err = ERR_SVC_REQUEST_TIMEOUT;
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
        if (atomic_load(&(volume->dispatched_count)) >= volume->concurrency_) {
            std::unique_lock<std::mutex> lk(concurrency_mutex);
            if (concurrency_cond.wait_for(lk, std::chrono::milliseconds(10000),
                [volume](){
                return (atomic_load(&(volume->dispatched_count)) < volume->concurrency_);})) {
                GLOGTRACE << "Will send IO: " << atomic_load(&(volume->dispatched_count));
            } else {
                GLOGWARN << "Timeout waiting for request to complete";
                return ERR_SVC_REQUEST_TIMEOUT;  // timeout should not happen :(
            }
        }

        ObjectID oid = (volume->testdata_).dataset_[i];
        boost::shared_ptr<const std::string> objData;
        err = getSm((volume->voldesc_).volUUID, oid, objData);
        if (!err.ok()) return err;
        dispatched = atomic_fetch_add(&(volume->dispatched_count), (fds_uint32_t)1);
    }

    // wait till all reads complete
    std::unique_lock<std::mutex> lk(done_mutex);
    if (done_cond.wait_for(lk, std::chrono::milliseconds(10000),
              [volume](){return (atomic_load(&(volume->dispatched_count)) < 1);})) {
        GLOGNOTIFY << "Finished validating store...";
    } else {
        GLOGWARN << "Timeout waiting for all reads to complete";
        err = ERR_SVC_REQUEST_TIMEOUT;
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
    storMgr->objectStore->handleNewDlt(dlt);

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

    if (dropCaches) {
      const std::string cmd = "echo 3 > /proc/sys/vm/drop_caches";
      int ret = std::system((const char *)cmd.c_str());
      fds_verify(ret == 0);
      std::cout << "sleeping after drop_caches" << std::endl;
      sleep(5);
      std::cout << "finished sleeping" << std::endl;
    }

    if (enableProfiler) {
        ProfilerStart("/tmp/SM_output.prof");
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

    if (enableProfiler) {
        ProfilerStop();
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
    fds_uint32_t cur_ind = 0;
    TestVolume::ptr volume = volumes_.volmap[volId];

    std::cout << "Starting thread for volume " << volId
              << " current number of ops finished " << ops << std::endl;

    fds_uint64_t start_nano = util::getTimeStampNanos();
    while ((ops + 1) <= volume->num_ops_)
    {
        if (atomic_load(&(volume->dispatched_count)) >= volume->concurrency_) {
            std::unique_lock<std::mutex> lk(concurrency_mutex);
            if (concurrency_cond.wait_for(lk, std::chrono::milliseconds(10000),
                       [volume](){return
                       (atomic_load(&(volume->dispatched_count)) < volume->concurrency_);})) {
                GLOGTRACE << "Will send IO: " << atomic_load(&(volume->dispatched_count));
            } else {
                GLOGWARN << "Timeout waiting for request to complete";
                err = ERR_SVC_REQUEST_TIMEOUT;  // timeout should not happen :(
                break;
            }
        }

        // do op
        fds_uint32_t index = cur_ind % (volume->testdata_).dataset_.size();
        ++cur_ind;
        if (volume->op_type_ == TestVolume::STORE_OP_GET) {
            cur_ind += 122;  // non-sequential access
        }
        ObjectID oid = (volume->testdata_).dataset_[index];
        switch (volume->op_type_) {
            case TestVolume::STORE_OP_PUT:
            case TestVolume::STORE_OP_DUPLICATE:
                {
                    boost::shared_ptr<std::string> data =
                            (volume->testdata_).dataset_map_[oid].getObjectData();
                    err = putSm(volId, oid, data);
                    break;
                }
            case TestVolume::STORE_OP_GET:
                {
                    boost::shared_ptr<const std::string> data;
                    err = getSm(volId, oid, data);
                    break;
                }
            case TestVolume::STORE_OP_DELETE:
                {
                    err = removeSm(volId, oid);
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
    double ave_lat = 0;
    fds_uint64_t lat_counter = 0;
    if (volume->op_type_ == TestVolume::STORE_OP_GET) {
        lat_counter = atomic_load(&get_lat_micro);
    } else if (volume->op_type_ == TestVolume::STORE_OP_PUT) {
        lat_counter = atomic_load(&put_lat_micro);
    }
    double total_ops = ops;
    if (total_ops > 0) {
        ave_lat = lat_counter / total_ops;
    }
    if (time_sec < 5) {
        std::cout << "Volume " << volId << " executed " << ops << " OPS, "
                  << "Experiment ran for too short time to calc IOPS" << std::endl;
        LOGNOTIFY << "Volume " << volId << " executed "<< ops << " OPS, "
                  << "Experiment ran for too short time to calc IOPS";
    } else {
        std::cout << "Volume " << volId << " executed " << ops << " OPS, "
                  << "Average IOPS = " << ops / time_sec << " latency "
                  << ave_lat << " microsec" << std::endl;
        LOGNOTIFY << "Volume " << volId << " executed "<< ops << " OPS, "
                  << "Average IOPS = " << ops / time_sec << " latency "
                  << ave_lat << " microsec";
    }
}

Error
SmLoadProc::putSm(fds_volid_t volId,
                  const ObjectID& objId,
                  boost::shared_ptr<const std::string> objData) {
    Error err(ERR_OK);

    boost::shared_ptr<fpi::PutObjectMsg> putObjMsg(new fpi::PutObjectMsg());
    putObjMsg->volume_id = volId.get();
    putObjMsg->data_obj = *objData;
    putObjMsg->data_obj_len = (*objData).size();
    putObjMsg->data_obj_id.digest =
            std::string((const char *)objId.GetId(), (size_t)objId.GetLen());

    auto putReq = new SmIoPutObjectReq(putObjMsg);
    putReq->io_type = FDS_SM_PUT_OBJECT;
    putReq->setVolId(volId);
    putReq->dltVersion = 1;
    putReq->forwardedReq = false;
    putReq->setObjId(objId);
    putReq->opReqFailedPerfEventType = PerfEventType::SM_PUT_OBJ_REQ_ERR;
    putReq->opReqLatencyCtx.type = PerfEventType::SM_E2E_PUT_OBJ_REQ;
    putReq->opReqLatencyCtx.reset_volid(volId);
    putReq->opLatencyCtx.type = PerfEventType::SM_PUT_IO;
    putReq->opLatencyCtx.reset_volid(volId);
    putReq->opQoSWaitCtx.type = PerfEventType::SM_PUT_QOS_QUEUE_WAIT;
    putReq->opQoSWaitCtx.reset_volid(volId);

    putReq->response_cb= std::bind(
        &SmLoadProc::putSmCb, this,
        std::placeholders::_1, std::placeholders::_2);

    err = storMgr->enqueueMsg(putReq->getVolId(), putReq);
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
    if (dispatched <= 1) {
        done_cond.notify_all();
    }
    if (dispatched <= vol->concurrency_) {
        concurrency_cond.notify_all();
    }

    // record stat
    fds_uint64_t lat_nano = util::getTimeStampNanos() - putReq->enqueue_ts;
    double lat_micro = lat_nano / 1000.00;
    fds_uint64_t lat = atomic_fetch_add(&put_lat_micro, (fds_uint64_t)lat_micro);
    delete putReq;
}

Error
SmLoadProc::getSm(fds_volid_t volId,
                  const ObjectID& objId,
                  boost::shared_ptr<const std::string>& objData) {
    Error err(ERR_OK);

    boost::shared_ptr<fpi::GetObjectMsg> getObjMsg(new fpi::GetObjectMsg());
    getObjMsg->volume_id = volId.get();
    getObjMsg->data_obj_id.digest = std::string((const char *)objId.GetId(),
                                                (size_t)objId.GetLen());

    auto getReq = new SmIoGetObjectReq(getObjMsg);
    getReq->io_type = FDS_SM_GET_OBJECT;
    getReq->setVolId(volId);
    getReq->setObjId(objId);
    getReq->obj_data.obj_id = getObjMsg->data_obj_id;
    getReq->opReqFailedPerfEventType = PerfEventType::SM_GET_OBJ_REQ_ERR;
    getReq->opReqLatencyCtx.type = PerfEventType::SM_E2E_GET_OBJ_REQ;
    getReq->opReqLatencyCtx.reset_volid(volId);
    getReq->opLatencyCtx.type = PerfEventType::SM_GET_IO;
    getReq->opLatencyCtx.reset_volid(volId);
    getReq->opQoSWaitCtx.type = PerfEventType::SM_GET_QOS_QUEUE_WAIT;
    getReq->opQoSWaitCtx.reset_volid(volId);

    getReq->response_cb = std::bind(
        &SmLoadProc::getSmCb, this,
        std::placeholders::_1, std::placeholders::_2);

    err = storMgr->enqueueMsg(getReq->getVolId(), getReq);
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
    if (dispatched <= 1) {
        done_cond.notify_all();
    }
    if (dispatched <= vol->concurrency_) {
        concurrency_cond.notify_all();
    }

    // record stat
    if (!validating_) {
        fds_uint64_t lat_nano = util::getTimeStampNanos() - getReq->enqueue_ts;
        double lat_micro = lat_nano / 1000.00;
        fds_uint64_t lat = atomic_fetch_add(&get_lat_micro, (fds_uint64_t)lat_micro);
    }
    delete getReq;
}


Error
SmLoadProc::removeSm(fds_volid_t volId,
                     const ObjectID& objId) {
    Error err(ERR_OK);

    boost::shared_ptr<fpi::DeleteObjectMsg> expObjMsg(new fpi::DeleteObjectMsg());
    expObjMsg->volId = volId.get();
    auto delReq = new SmIoDeleteObjectReq(expObjMsg);
    delReq->io_type = FDS_SM_DELETE_OBJECT;
    delReq->setVolId(volId);
    delReq->dltVersion = 1;
    delReq->forwardedReq = false;
    delReq->setObjId(ObjectID(expObjMsg->objId.digest));
    delReq->opReqFailedPerfEventType = PerfEventType::SM_DELETE_OBJ_REQ_ERR;
    delReq->opReqLatencyCtx.type = PerfEventType::SM_E2E_DELETE_OBJ_REQ;
    delReq->opLatencyCtx.type = PerfEventType::SM_DELETE_IO;
    delReq->opQoSWaitCtx.type = PerfEventType::SM_DELETE_QOS_QUEUE_WAIT;

    delReq->response_cb = std::bind(
        &SmLoadProc::removeSmCb, this,
        std::placeholders::_1, std::placeholders::_2);

    err = storMgr->enqueueMsg(delReq->getVolId(), delReq);
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
    if (dispatched <= 1) {
        done_cond.notify_all();
    }
    if (dispatched <= vol->concurrency_) {
        concurrency_cond.notify_all();
    }

    delete delReq;
}

Error
SmLoadProc::regVolume(TestVolume::ptr& volume) {
    FDSP_NotifyVolFlag vol_flag = FDSP_NOTIFY_VOL_NO_FLAG;
    fds_volid_t volumeId = volume->voldesc_.volUUID;
    Error err = storMgr->regVol(volume->voldesc_);
    if (err.ok()) {
        StorMgrVolume * vol = storMgr->getVol(volumeId);
        fds_assert(vol != NULL);

        fds_volid_t queueId = vol->getQueue()->getVolUuid();
        if (!storMgr->getQueue(queueId)) {
            err = storMgr->regVolQos(queueId, static_cast<FDS_VolumeQueue*>(
                vol->getQueue().get()));
        }

        if (!err.ok()) {
            storMgr->deregVol(volumeId);
        }
    }
    if (!err.ok()) {
        GLOGERROR << "Registration failed for vol id " << std::hex << volumeId
                  << std::dec << " error: " << err.GetErrstr();
    }
    return err;
}

}  // namespace fds

int
main(int argc, char * argv[])
{
    storMgr = new fds::ObjectStorMgr(g_fdsprocess);
    objStorMgr  = storMgr;
    std::cout << "Will test SM" << std::endl;
    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        storMgr,
        nullptr
    };
    fds::SmLoadProc p(argc, argv, "platform.conf", "fds.sm.", smVec);

    std::cout << "unit test " << __FILE__ << " started." << std::endl;
    p.main();

    delete storMgr;
    storMgr = NULL;
    std::cout << "unit test " << __FILE__ << " finished." << std::endl;
    return 0;
}


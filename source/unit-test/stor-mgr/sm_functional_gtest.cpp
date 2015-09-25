/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cstdio>
#include <string>
#include <vector>
#include <bitset>

#include <boost/program_options.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fdsp/sm_types_types.h>
#include <util/Log.h>
#include <fdsp_utils.h>
#include <fds_types.h>
#include <ObjectId.h>
#include <fds_module.h>
#include <fds_process.h>
#include <StorMgr.h>
#include <sm_dataset.h>
#include <sm_ut_utils.h>
#include <SMSvcHandler.h>
#include <net/SvcProcess.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

static ObjectStorMgr* sm;

static fds_uint32_t hddCount = 0;
static fds_uint32_t ssdCount = 10;
static TestVolume::ptr volume1;

class SmUnitTestProc : public SvcProcess {
  public:
    SmUnitTestProc(int argc, char *argv[],
                   Module **mod_vec) :
            SvcProcess(argc,
                       argv,
                       "platform.conf",
                       "fds.sm.",
                       "sm-unit-test.log",
                       mod_vec) {
        sm->setModProvider(this);
    }
    ~SmUnitTestProc() {
    }
    int run() override {
        return 0;
    }
};

class SmUnitTest: public ::testing::Test {
  public:
    SmUnitTest()
            : numOps(0), expectedGetError(ERR_OK),
              expectedPutError(ERR_OK) {
        op_count = ATOMIC_VAR_INIT(0);
    }
    ~SmUnitTest() {
    }

    virtual void TearDown() override;
    void task(TestVolume::StoreOpType opType,
              TestVolume::ptr volume);
    void runMultithreadedTest(TestVolume::StoreOpType opType,
                              TestVolume::ptr volume,
                              fds_bool_t waitIoComplete);
    static void setupTests(fds_uint32_t concurrency,
                           fds_uint32_t datasetSize);
    void handleDlt();
    void addVolume(TestVolume::ptr volume);
    Error getSm(fds_volid_t volId,
                const ObjectID& objId,
                boost::shared_ptr<const std::string>& objData);
    Error putSm(fds_volid_t volId,
                const ObjectID& objId,
                boost::shared_ptr<const std::string> objData);

    void shutdownAndWaitIoComplete(TestVolume::ptr volume);

    // callbacks
    void putSmCb(const Error &err,
                 SmIoPutObjectReq* putReq);
    void getSmCb(const Error &err,
                 SmIoGetObjectReq *getReq);

    std::vector<std::thread*> threads_;
    std::atomic<fds_uint32_t> op_count;

    // this has to be initialized before running
    // multi-threaded tests
    fds_uint32_t numOps;

    // to ensure configured concurrency
    std::condition_variable concurrency_cond;
    std::mutex concurrency_mutex;

    // so we know when we are done writing/reading
    std::condition_variable done_cond;
    std::mutex done_mutex;

    // expected errors for data path API in addition ERR_OK
    // normally would be ERR_OK
    Error expectedPutError;
    Error expectedGetError;
};

void SmUnitTest::TearDown() {
}

void SmUnitTest::setupTests(fds_uint32_t concurrency,
                            fds_uint32_t datasetSize) {
    fds_volid_t volId(98);

    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    const std::string devPath = dir->dir_dev();
    SmUtUtils::cleanAllInDir(devPath);

    // create our own disk map
    SmUtUtils::setupDiskMap(dir, hddCount, ssdCount);

    sm->standaloneTestDlt = NULL;
    sm->mod_init(NULL);
    sm->mod_startup();
    sm->mod_enable_service();

    // volume for single-volume test
    // we will ignore optype so just set put
    volume1.reset(new TestVolume(volId, "sm_ut_vol",
                                 concurrency, 0,
                                 TestVolume::STORE_OP_PUT,
                                 datasetSize, 4096));
}

Error
SmUnitTest::putSm(fds_volid_t volId,
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
        &SmUnitTest::putSmCb, this,
        std::placeholders::_1, std::placeholders::_2);

    err = sm->enqueueMsg(putReq->getVolId(), putReq);
    if (err != fds::ERR_OK) {
        LOGERROR << "Failed to enqueue to SmIoPutObjectReq to StorMgr.  Error: "
                 << err;
    }
    LOGDEBUG << err;
    return err;
}

Error
SmUnitTest::getSm(fds_volid_t volId,
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
        &SmUnitTest::getSmCb, this,
        std::placeholders::_1, std::placeholders::_2);

    err = sm->enqueueMsg(getReq->getVolId(), getReq);
    if (err != fds::ERR_OK) {
        LOGERROR << "Failed to enqueue to SmIoReadObjectMetadata to StorMgr.  Error: "
                 << err;
    }
    LOGDEBUG << err;
    return err;
}

void
SmUnitTest::putSmCb(const Error &err,
                    SmIoPutObjectReq* putReq)
{
    fds_volid_t volId = putReq->getVolId();
    ASSERT_EQ(volId.get(), 98u);
    fds_uint32_t dispatched = atomic_fetch_sub(&(volume1->dispatched_count), (fds_uint32_t)1);
    if (dispatched <= 1) {
        done_cond.notify_all();
    }
    if (dispatched <= volume1->concurrency_) {
        concurrency_cond.notify_all();
    }

    delete putReq;
}

void
SmUnitTest::getSmCb(const Error &err,
                    SmIoGetObjectReq *getReq)
{
    fds_volid_t volId = getReq->getVolId();
    ASSERT_EQ(volId.get(), 98u);
    TestVolume::ptr vol = volume1;

    // validate data
    if (err.ok()) {
        const ObjectID& oid = getReq->getObjId();
        boost::shared_ptr<std::string> data(new std::string(getReq->getObjectNetResp->data_obj));
        LOGDEBUG << "Verifying object " << oid;
        EXPECT_TRUE((vol->testdata_).dataset_map_[oid].isValid(data));
    } else {
        EXPECT_EQ(err, expectedGetError);
        LOGDEBUG << err;
    }

    fds_uint32_t dispatched = atomic_fetch_sub(&(vol->dispatched_count), (fds_uint32_t)1);
    if (dispatched <= 1) {
        done_cond.notify_all();
    }
    if (dispatched <= vol->concurrency_) {
        concurrency_cond.notify_all();
    }

    delete getReq;
}

void SmUnitTest::task(TestVolume::StoreOpType opType,
                      TestVolume::ptr volume) {
    Error err(ERR_OK);
    fds_uint32_t dispatched = 0;
    fds_uint64_t seed = RandNumGenerator::getRandSeed();
    RandNumGenerator rgen(seed);
    fds_uint32_t datasetSize = (volume->testdata_).dataset_.size();
    fds_volid_t volId = (volume->voldesc_).volUUID;
    fds_uint32_t ops = atomic_fetch_add(&op_count, (fds_uint32_t)1);

    while ((ops + 1) <= numOps) {
        fds_uint32_t index = ops % datasetSize;
        if ((opType == TestVolume::STORE_OP_GET) ||
            (opType == TestVolume::STORE_OP_DUPLICATE)) {
            index = ((fds_uint32_t)rgen.genNum()) % datasetSize;
        }

        if (atomic_load(&(volume->dispatched_count)) >= volume->concurrency_) {
            std::unique_lock<std::mutex> lk(concurrency_mutex);
            if (concurrency_cond.wait_for(lk, std::chrono::milliseconds(10000),
                                          [volume](){return
                                                      (atomic_load(&(volume->dispatched_count)) < volume->concurrency_);})) {
                GLOGTRACE << "Will send IO: " << atomic_load(&(volume->dispatched_count));
            } else {
                GLOGWARN << "Timeout waiting for request to complete";
                err = ERR_SVC_REQUEST_TIMEOUT;
                break;
            }
        }
        EXPECT_TRUE(err.ok());

        ObjectID oid = (volume->testdata_).dataset_[index];
        switch (opType) {
            case TestVolume::STORE_OP_PUT:
            case TestVolume::STORE_OP_DUPLICATE:
                {
                    boost::shared_ptr<std::string> data =
                            (volume->testdata_).dataset_map_[oid].getObjectData();
                    err = putSm(volId, oid, data);
                    EXPECT_TRUE((err.ok()) ||
                                (err == expectedPutError));
                }
                break;
            case TestVolume::STORE_OP_GET:
                {
                    boost::shared_ptr<const std::string> data;
                    err = getSm(volId, oid, data);
                    EXPECT_TRUE(err.ok() || (err == expectedGetError));
                }
                break;
            case TestVolume::STORE_OP_DELETE:
                // err = objectStore->deleteObject(volId, oid, false);
                EXPECT_TRUE(err.ok());
                break;
            default:
                fds_verify(false);   // new type added?
        };
        ops = atomic_fetch_add(&op_count, (fds_uint32_t)1);
        if (err.ok()) {
            dispatched = atomic_fetch_add(&(volume->dispatched_count), (fds_uint32_t)1);
        } else {
            err = ERR_OK;
        }
    }
}

void
SmUnitTest::runMultithreadedTest(TestVolume::StoreOpType opType,
                                 TestVolume::ptr volume,
                                 fds_bool_t waitIoComplete) {
    Error err(ERR_OK);
    GLOGDEBUG << "Will do multi-threded op type " << opType
              << " volume " << (volume->voldesc_).name
              << " dataset size " << (volume->testdata_).dataset_.size()
              << " num ops " << numOps
              << " concurrency " << volume->concurrency_;

    // setup the run
    EXPECT_EQ(threads_.size(), 0u);
    op_count = ATOMIC_VAR_INIT(0);

    for (unsigned i = 0; i < volume->concurrency_; ++i) {
        std::thread* new_thread = new std::thread(&SmUnitTest::task, this,
                                                  opType, volume);
        threads_.push_back(new_thread);
    }
    // wait for all threads
    for (unsigned x = 0; x < volume->concurrency_; ++x) {
        threads_[x]->join();
    }

    // even though threads may have finished sending IO,
    // wait for all IO to actually complete
    if (waitIoComplete) {
        std::unique_lock<std::mutex> lk(done_mutex);
        if (done_cond.wait_for(lk, std::chrono::milliseconds(10000),
                               [volume](){return (atomic_load(&(volume->dispatched_count)) < 1);})) {
            GLOGNOTIFY << "Finished with multi-threaded ops ...";
        } else {
            GLOGWARN << "Timeout waiting for all ops to complete";
            err = ERR_SVC_REQUEST_TIMEOUT;
        }
        EXPECT_TRUE(err.ok());
    }

    // cleanup
    for (unsigned x = 0; x < volume->concurrency_; ++x) {
        std::thread* th = threads_[x];
        delete th;
        threads_[x] = NULL;
    }
    threads_.clear();
}

void
SmUnitTest::shutdownAndWaitIoComplete(TestVolume::ptr volume) {
    Error err(ERR_OK);

    LOGNOTIFY << "Calling prepare for shutdown";
    sm->mod_disable_service();
    sm->mod_shutdown();
    LOGNOTIFY << "SM finished handling prepare for shutdown";

    // even though threads may have finished sending IO,
    // wait for all IO to actually complete
    std::unique_lock<std::mutex> lk(done_mutex);
    if (done_cond.wait_for(lk, std::chrono::milliseconds(10000),
                           [volume](){return (atomic_load(&(volume->dispatched_count)) < 1);})) {
        GLOGNOTIFY << "Finished with multi-threaded ops ...";
    } else {
        GLOGWARN << "Timeout waiting for all ops to complete";
        err = ERR_SVC_REQUEST_TIMEOUT;
    }
    EXPECT_TRUE(err.ok());
}

void SmUnitTest::handleDlt() {
    Error err(ERR_OK);
    fds_uint32_t sm_count = 1;
    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;

    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    err = sm->objectStore->handleNewDlt(dlt);
    EXPECT_TRUE(err.ok());
    LOGTRACE << "SM now has a new DLT";

    if (sm->standaloneTestDlt) {
        delete sm->standaloneTestDlt;
        sm->standaloneTestDlt = NULL;
    }
    sm->setTestDlt(dlt);
}

void SmUnitTest::addVolume(TestVolume::ptr volume)
{
    Error err(ERR_OK);
    fds_volid_t volumeId(98);
    FDSP_NotifyVolFlag vol_flag = FDSP_NOTIFY_VOL_NO_FLAG;
    FDSP_ResultType result = FDSP_ERR_OK;
    err = sm->registerVolume(volumeId, &volume1->voldesc_, vol_flag, result);
    EXPECT_TRUE(err.ok());
}

TEST_F(SmUnitTest, handle_dlt) {
    handleDlt();
}

TEST_F(SmUnitTest, volume_add) {
    addVolume(volume1);
}

TEST_F(SmUnitTest, concurrent_puts) {
    Error err(ERR_OK);
    GLOGDEBUG << "Running concurrent_puts test";
    numOps = (volume1->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_PUT, volume1, true);
}

TEST_F(SmUnitTest, validate_initial_put) {
    Error err(ERR_OK);
    GLOGDEBUG << "Running concurrent_gets test";
    numOps = (volume1->testdata_).dataset_.size();        
    // this test also validates
    runMultithreadedTest(TestVolume::STORE_OP_GET, volume1, true);
}

TEST_F(SmUnitTest, prepare_for_shutdown) {
    Error err(ERR_OK);

    // start doing reads but do not wait for all of them to complete
    numOps = (volume1->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_GET, volume1, false);

    // prepare for shutdown and after that, wait that all IO that
    // were in flight complete either successful or with error
    expectedGetError = ERR_NOT_READY;
    shutdownAndWaitIoComplete(volume1);

    // start a new set of IO, these should return error
    LOGNOTIFY << "Will do IO while SM shutting down, expecting errors";

    // at this point IO should return errors
    numOps = (volume1->testdata_).dataset_.size();
    // don't do too many IOs... just a few
    if (numOps > volume1->concurrency_) {
        numOps = volume1->concurrency_;
    }
    expectedPutError = ERR_NOT_READY;
    runMultithreadedTest(TestVolume::STORE_OP_PUT, volume1, true);

    // back to original test state
    expectedGetError = ERR_OK;
    expectedPutError = ERR_OK;

    // try to shutdown again, nothing should happen
    LOGNOTIFY << "Will do second prepare for shutdown msg";
    sm->mod_disable_service();
    sm->mod_shutdown();
}

}  // namespace fds

int
main(int argc, char** argv) {
    int ret = 0;
    sm = new fds::ObjectStorMgr(g_fdsprocess);
    objStorMgr  = sm;
    fds::SmUnitTestProc p(argc, argv, NULL);
    ::testing::InitGoogleMock(&argc, argv);

    namespace po = boost::program_options;
    po::options_description progDesc("SM Unit Test options");
    progDesc.add_options()
            ("help,h", "Help Message")
            ("hdd-count",
             po::value<fds_uint32_t>(&hddCount)->default_value(12),
             "Number of HDDs")
            ("ssd-count",
             po::value<fds_uint32_t>(&ssdCount)->default_value(2),
             "Number of SSDs");
    po::variables_map varMap;
    po::parsed_options parsedOpt =
            po::command_line_parser(argc, argv).options(progDesc).allow_unregistered().run();                                                                               
    po::store(parsedOpt, varMap);
    po::notify(varMap);

    std::cout << "Disk map config: " << hddCount << " HDDs, "
              << ssdCount << " SSDs" << std::endl;

    SmUnitTest::setupTests(40, 30);

    ret = RUN_ALL_TESTS();

    // cleanup
    delete sm;
    sm = NULL;
    SmUtUtils::cleanAllInDir(g_fdsprocess->proc_fdsroot()->dir_dev());

    return ret;
}

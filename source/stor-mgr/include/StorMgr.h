/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_STORMGR_H_
#define SOURCE_STOR_MGR_INCLUDE_STORMGR_H_

#include <atomic>
#include <string>
#include <queue>
#include <unordered_map>
#include <utility>

#include "fdsp/FDSP_types.h"
#include "fdsp/health_monitoring_types_types.h"
#include "fds_types.h"
#include "ObjectId.h"
#include "util/Log.h"
#include "util/always_call.h"
#include "StorMgrVolumes.h"
#include "persistent-layer/dm_io.h"
#include "hash/md5.h"

#include "fds_qos.h"
#include "qos_ctrl.h"
#include "fds_assert.h"
#include "fds_config.hpp"
#include "util/timeutils.h"
#include "lib/StatsCollector.h"

/*
 * TODO: Move this header out of lib/
 * to include/ since it's linked by many.
 */
#include "lib/qos_htb.h"
#include "lib/qos_min_prio.h"
#include "lib/QoSWFQDispatcher.h"

#include "fds_module.h"

#include "fdsp/SMSvc.h"

#include <object-store/ObjectStore.h>
#include <MigrationMgr.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>
#include <fdsp/event_types_types.h>


#define FDS_STOR_MGR_LISTEN_PORT FDS_CLUSTER_TCP_PORT_SM
#define FDS_STOR_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM
#define FDS_MAX_WAITING_CONNS  10

using namespace FDS_ProtocolInterface;  // NOLINT

namespace fds {

extern ObjectStorMgr *objStorMgr;

/* File names for storing DLT and UUID.  Mainly used by smchk to
 * determine proper token ownership.
 */
const std::string DLTFileName = "/currentDLT";
const std::string UUIDFileName = "/uuidDLT";

class ObjectStorMgr : public Module, public SmIoReqHandler {
    protected:
     typedef enum {
         NORMAL_MODE = 0,
         TEST_MODE   = 1,
         MAX
     } SmRunModes;

     CommonModuleProviderIf *modProvider_;
     /*
      * glocal dedupe  stats  counter
      */

     std::atomic<fds_uint64_t> dedupeByteCnt;

     /// Manager of persistent object storage
     ObjectStore::unique_ptr objectStore;
     /// Manager of token migration
     MigrationMgr::unique_ptr migrationMgr;

     /*
      * TODO: this one should be the singleton by itself.  Need to make it
      * a stand-alone module like resource manager for volume.
      * Volume specific members
      */
     StorMgrVolumeTable *volTbl;

     /*
      * Qos related members and classes
      */
     fds_uint32_t totalRate;
     fds_uint32_t qosThrds;
     fds_uint32_t qosOutNum;

     // true if running SM standalone (for testing)
     fds_bool_t testStandalone;

    /// True if request serialization is enabled
    bool enableReqSerialization;

     class SmQosCtrl : public FDS_QoSControl {
        private:
         ObjectStorMgr *parentSm;

         // Defines a serialization key based on volume ID and client
         // service ID
         using SerialKey = std::pair<fds_volid_t, fpi::SvcUuid>;
         static const std::hash<fds_volid_t> volIdHash;
         static const std::hash<int64_t> svcIdHash;
         struct SerialKeyHash {
            size_t operator()(const SerialKey &key) const {
                return volIdHash(key.first) + svcIdHash(key.second.svc_uuid);
            }
        };
        static const SerialKeyHash keyHash;

         /// Enables request serialization for a generic size_t key
         /// Using size allows different keys to be used by the same
         /// executor.
         std::unique_ptr<SynchronizedTaskExecutor<size_t>> serialExecutor;

        public:
         SmQosCtrl(ObjectStorMgr *_parent,
                   uint32_t _max_thrds,
                   dispatchAlgoType algo,
                   fds_log *log) :
             FDS_QoSControl(_max_thrds, algo, log, "SM") {
                 parentSm = _parent;
                 LOGNOTIFY << "Qos totalRate " << parentSm->totalRate
                           << ", num outstanding io " << parentSm->qosOutNum
                           << ", qos threads " << _max_thrds;

                 // dispatcher = new QoSMinPrioDispatcher(this, log, 3000);
                 dispatcher = new QoSWFQDispatcher(this,
                                                   parentSm->totalRate,
                                                   parentSm->qosOutNum,
                                                   log);
                 // dispatcher = new QoSHTBDispatcher(this, log, 150);

                 serialExecutor = std::unique_ptr<SynchronizedTaskExecutor<size_t>>(
                     new SynchronizedTaskExecutor<size_t>(*threadPool));
             }
         virtual ~SmQosCtrl() {
             delete dispatcher;
         }

         Error processIO(FDS_IOType* _io);

         Error markIODone(FDS_IOType& _io) {
             Error err(ERR_OK);
             dispatcher->markIODone(&_io);
             return err;
         }

         Error markIODone(FDS_IOType &_io,
                          diskio::DataTier  tier,
                          fds_bool_t iam_primary = false) {
             Error err(ERR_OK);
             dispatcher->markIODone(&_io);
             if (iam_primary &&
                 (_io.io_type == FDS_SM_GET_OBJECT) &&
                 (tier == diskio::flashTier)) {
                     StatsCollector::singleton()->recordEvent(_io.io_vol_id,
                                                              _io.io_done_ts,
                                                              STAT_SM_GET_SSD,
                                                              _io.io_total_time);
             }
             return err;
         }
     };


     SmVolQueue *sysTaskQueue;
     std::atomic_bool  shuttingDown;      /* SM shut down flag for write-back thread */
     SysParams *sysParams;

     /**
      * should be just enough to make sure system task makes progress
      */
     inline fds_uint32_t getSysTaskIopsMin() {
         return 10;
     }

     /**
      * We should not care about max iops -- if there is no other work
      * in SM, we should be ok to take all available bandwidth
      */
     inline fds_uint32_t getSysTaskIopsMax() {
         return 0;
     }

     inline fds_uint32_t getSysTaskPri() {
         return FdsSysTaskPri;
     }

  public:
    SmQosCtrl  *qosCtrl;
    explicit ObjectStorMgr(CommonModuleProviderIf *modProvider);
     /* This constructor is exposed for mock testing */
     ObjectStorMgr()
             : Module("sm"), totalRate(20000) {
         qosCtrl = nullptr;
         volTbl = nullptr;
     }
     /* this is for standalone testing */
     void setModProvider(CommonModuleProviderIf *modProvider);

     ~ObjectStorMgr();

     /* Overrides from Module */
     virtual int  mod_init(SysParams const *const param) override;
     virtual void mod_startup() override;
     virtual void mod_shutdown() override;
     virtual void mod_enable_service() override;
     virtual void mod_disable_service() override;

     int run();

     /// Enables uturn testing for all sm service ops
     fds_bool_t testUturnAll;
     /// Enables uturn testing for put object ops
     fds_bool_t testUturnPutObj;

     fds_bool_t isShuttingDown() const {
         return shuttingDown;
     }

     Error regVol(const VolumeDesc& vdb) {
         return volTbl->registerVolume(vdb);
     }

    Error deregVol(fds_volid_t volId) {
        return volTbl->deregisterVolume(volId);
    }

    void quieseceIOsQos(fds_volid_t volId) {
        qosCtrl->quieseceIOs(volId);
    }

    Error modVolQos(fds_volid_t vol_uuid,
                    fds_int64_t iops_assured,
                    fds_int64_t iops_throttle,
                    fds_uint32_t prio) {
        return qosCtrl->modifyVolumeQosParams(vol_uuid,
                    iops_assured, iops_throttle, prio);
    }

    StorMgrVolume * getVol(fds_volid_t volId)
    {
        return volTbl->getVolume(volId);
    }


    Error regVolQos(fds_volid_t volId, fds::FDS_VolumeQueue * q) {
        return qosCtrl->registerVolume(volId, q);
    }
    Error deregVolQos(fds_volid_t volId) {
        return qosCtrl->deregisterVolume(volId);
    }
    SmVolQueue * getQueue(fds_volid_t volId) const {
        return static_cast<SmVolQueue*>(qosCtrl->getQueue(volId));
    }

     // We need to get this info out of this big class to avoid making this
     // class even bigger than it should.  Not much point for making it
     // private and need a get method to get it out.
     //
     StorMgrVolumeTable *sm_getVolTables() {
         return volTbl;
     }

     /**
      * A callback from stats collector to sample SM specific stats
      * @param timestamp is timestamp to path to every recordEvent()
      * method to stats collector when recording SM stats
      */
     void sampleSMStats(fds_uint64_t timestamp);

     nullary_always
     getTokenLock(ObjectID const& id, bool exclusive = false)
     {
         fds_uint32_t b_p_t = getDLT()->getNumBitsForToken();
         return getTokenLock(SmDiskMap::smTokenId(id, b_p_t), exclusive);
     }

     nullary_always
     getTokenLock(fds_token_id const& id, bool exclusive = false);

     void getObjectInternal(SmIoGetObjectReq *getReq);
     void putObjectInternal(SmIoPutObjectReq* putReq);
     void deleteObjectInternal(SmIoDeleteObjectReq* delReq);
     void addObjectRefInternal(SmIoAddObjRefReq* addRefReq);

     void snapshotTokenInternal(SmIoReq* ioReq);
     void compactObjectsInternal(SmIoReq* ioReq);
     void moveTierObjectsInternal(SmIoReq* ioReq);
     void applyRebalanceDeltaSet(SmIoReq* ioReq);
     void readObjDeltaSet(SmIoReq* ioReq);
     void abortMigration(SmIoReq* ioReq);
     void notifyDLTClose(SmIoReq* ioReq);
     void startResyncRequest();
     Error handleDltUpdate();

     void storeCurrentDLT();

     virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* ioReq) override;

     /* Made virtual for google mock */
     TVIRTUAL const DLT* getDLT();

     const std::string getStorPrefix() {
         return modProvider_->get_fds_config()->get<std::string>("fds.sm.prefix");
     }

     NodeUuid getUuid() const;
     fds_bool_t amIPrimary(const ObjectID& objId);


     virtual std::string log_string() {
         std::stringstream ret;
         ret << " ObjectStorMgr";
         return ret.str();
     }

     friend class SmLoadProc;
     friend class SmUnitTest;
     friend class SMSvcHandler;

  private:
     void sendHealthCheckMsgToOM(fpi::HealthState serviceState,
                                 fds_errno_t statusCode,
                                 const std::string& statusInfo);

     void sendEventMessageToOM(fpi::EventType eventType,
                               fpi::EventCategory eventCategory,
                               fpi::EventSeverity eventSeverity,
                               fpi::EventState eventState,
                               const std::string& messageKey,
                               std::vector<fpi::MessageArgs> messageArgs,
                               const std::string& messageFormat);


     static Error registerVolume(fds::fds_volid_t volume_id,
                                 fds::VolumeDesc *vdb,
                                 FDSP_NotifyVolFlag vol_flag,
                                 FDSP_ResultType result);

     // for standalone test
     DLT* standaloneTestDlt;
     void setTestDlt(DLT* dlt) {
         standaloneTestDlt = dlt;
     }
    /**
     * Used for disk capacity tracking
     */

    // Tracks the number of time collect sample stats has run, to enable capacity tracking every N runs
    fds_uint8_t sampleCounter;
    // Track when the last message was sent
    float_t lastCapacityMessageSentAt;
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_STORMGR_H_

/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the data manager component.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DATAMGR_H_
#define SOURCE_DATA_MGR_INCLUDE_DATAMGR_H_

#include <list>
#include <vector>
#include <map>
#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <fds_timer.h>

/* TODO: avoid cross module include, move API header file to include dir. */

#include <util/Log.h>
#include <VolumeMeta.h>
#include <concurrency/ThreadPool.h>

#include <include/fds_qos.h>
#include <include/qos_ctrl.h>
#include <unordered_map>
#include <string>
#include <persistent-layer/dm_service.h>

#include <fdsp/event_types_types.h>
#include "fdsp/health_monitoring_types_types.h"
#include "fds_types.h"
#include <fdsp/FDSP_types.h>
#include <lib/QoSWFQDispatcher.h>
#include <lib/qos_min_prio.h>

#include <net/filetransferservice.h>
#include <dmhandler.h>
#include <DmIoReq.h>
#include <CatalogSync.h>
#include <CatalogSyncRecv.h>
#include <dm-tvc/CommitLog.h>

#include <blob/BlobTypes.h>
#include <fdsp/DMSvc.h>
#include <functional>

#include <dm-tvc/TimeVolumeCatalog.h>
#include <StatStreamAggregator.h>
#include <DataMgrIf.h>

#include <DmMigrationMgr.h>
#include "util/ExecutionGate.h"
#include <timeline/timelinemanager.h>
#include <expungemanager.h>
#include <refcount/refcountmanager.h>
/* if defined, puts complete as soon as they
 * arrive to DM (not for gets right now)
 */
#undef FDS_TEST_DM_NOOP



namespace fds {
namespace dm {
struct Handler;
struct Counters;
}
class DMSvcHandler;
class DmMigrationMgr;
struct DataMgr : HasModuleProvider, Module, DmIoReqHandler, DataMgrIf {
    static void InitMsgHdr(const fpi::FDSP_MsgHdrTypePtr& msg_hdr);

    /*
     * TODO: Move to STD shared or unique pointers. That's
     * safer.
     */
    std::unordered_map<fds_volid_t, VolumeMeta*> vol_meta_map;
    /**
     * Catalog sync manager
     */
    CatalogSyncMgrPtr catSyncMgr;  // sending vol meta
    CatSyncReceiverPtr catSyncRecv;  // receiving vol meta
    void initHandlers();
    VolumeMeta* getVolumeMeta(fds_volid_t volId, bool fMapAlreadyLocked = false);
    /**
    * Callback for DMT close
    */
    DmtCloseCb sendDmtCloseCb;

    /**
     * DmIoReqHandler method implementation
     */
    virtual Error enqueueMsg(fds_volid_t volId, DmRequest* ioReq) override;

    /**
     * If this DM, given the volume, is the TOP listed DM of the DMT column.
     */
    fds_bool_t amIPrimary(fds_volid_t volUuid);

    /**
     * If this DM, given the volume, is within the Primary group.
     */
    fds_bool_t amIPrimaryGroup(fds_volid_t volUuid);


    inline StatStreamAggregator::ptr statStreamAggregator() {
        return statStreamAggr_;
    }

    inline const std::string & volumeName(fds_volid_t volId) {
        FDSGUARD(vol_map_mtx);
        return vol_meta_map[volId]->vol_desc->name;
    }

    virtual const VolumeDesc * getVolumeDesc(fds_volid_t volId) const;
    void getActiveVolumes(std::vector<fds_volid_t>& vecVolIds);

    ///
    /// Check if a given volume is active.
    ///
    /// @param volumeId The ID of the volume to check.
    ///
    /// @return ERR_OK if the volume is active. ERR_VOL_NOT_FOUND if @p volumeId is not in the
    ///         volume map. ERR_DM_VOL_NOT_ACTIVATED if the volume exists but is not active.
    ///
    Error validateVolumeIsActive(fds_volid_t const volumeId) const {
        auto volumeDesc = getVolumeDesc(volumeId);
        if (!volumeDesc) {
            return ERR_VOL_NOT_FOUND;
        }

        if (volumeDesc->state != fpi::Active)
        {
            return ERR_DM_VOL_NOT_ACTIVATED;
        }

        return ERR_OK;
    }

    /**
     * Pull the volume descriptors from OM using service layer implementation.
     * Then populate the volumes into DM.
     */
    Error getAllVolumeDescriptors();

    Error process_rm_vol(fds_volid_t vol_uuid, fds_bool_t check_only);

    /**
    * @brief Detach in any in memory state for the volume
    *
    * @param vol_uuid
    */
    void detachVolume(fds_volid_t vol_uuid);

    typedef enum {
      NORMAL_MODE = 0,
      TEST_MODE   = 1,
      MAX
    } dmRunModes;
    dmRunModes    runMode;


#define DEF_FEATURE(name, defvalue)                             \
    private: bool f##name = defvalue;                           \
  public: inline bool is##name##Enabled() const {               \
      return f##name;                                           \
  }                                                             \
  public: inline void set##name##Enabled(bool const val) {      \
      f##name = val;                                            \
  }

    class Features {
        DEF_FEATURE(Qos          , true);
        DEF_FEATURE(CatSync      , true);
        DEF_FEATURE(Timeline     , true);
        DEF_FEATURE(VolumeTokens , false);
        DEF_FEATURE(SerializeReqs, true);
        DEF_FEATURE(TestMode     , false);
        DEF_FEATURE(Expunge      , true);
        DEF_FEATURE(Volumegrouping, false);
    } features;

    dm::Counters* counters;

    fds_uint32_t numTestVols;  /* Number of vols to use in test mode */
    boost::shared_ptr<timeline::TimelineManager> timelineMgr;
    boost::shared_ptr<ExpungeManager> expungeMgr;
    fds_threadpool  lowPriorityTasks;
    /**
     * For timing out request forwarding in DM (to send DMT close ack)
     */
    FdsTimerTaskPtr closedmt_timer_task;

    /**
     * Time Volume Catalog that provides update and query access
     * to volume catalog
     */
    DmTimeVolCatalog::ptr timeVolCat_;

    /**
     * Aggregator of volume stats streams
     */
    StatStreamAggregator::ptr statStreamAggr_;

    net::FileTransferService::ptr fileTransfer;
    refcount::RefCountManager::ptr refCountMgr;
    struct dmQosCtrl : FDS_QoSControl {
        DataMgr *parentDm;

        /// Defines a serialization key based on volume ID and blob name
        using SerialKey = std::pair<fds_volid_t, std::string>;
        static const std::hash<fds_volid_t> volIdHash;
        static const std::hash<std::string> blobNameHash;
        struct SerialKeyHash {
            size_t operator()(const SerialKey &key) const {
                return volIdHash(key.first) + blobNameHash(key.second);
            }
        };
        static const SerialKeyHash keyHash;

        /// Enables request serialization for a generic size_t key
        /// Using size allows different keys to be used by the same
        /// executor.
        std::unique_ptr<SynchronizedTaskExecutor<size_t>> serialExecutor;

        dmQosCtrl(DataMgr *_parent, uint32_t _max_thrds, dispatchAlgoType algo, fds_log *log);
        Error markIODone(const FDS_IOType& _io);
        Error processIO(FDS_IOType* _io);
        virtual ~dmQosCtrl();        
    };

    FDS_VolumeQueue*  sysTaskQueue;
    std::atomic_bool  shuttingDown;      /* SM shut down flag for write-back thread */

    /*
     * Cmdline configurables
     */
    std::string  stor_prefix;   /* String prefix to make file unique */
    fds_uint32_t  scheduleRate;
    fds_bool_t   standalone;    /* Whether to bootstrap from OM */

    std::string myIp;

    /*
     * Used to protect access to vol_meta_map.
     */
    fds_mutex *vol_map_mtx;

    Error getVolObjSize(fds_volid_t volId,
                        fds_uint32_t *maxObjSize);

    Error addVolume(const std::string& vol_name, fds_volid_t vol_uuid, VolumeDesc* desc);
    Error _process_mod_vol(fds_volid_t vol_uuid,
                           const VolumeDesc& voldesc);

    void initSmMsgHdr(fpi::FDSP_MsgHdrTypePtr msgHdr);

    fds_bool_t volExistsLocked(fds_volid_t vol_uuid) const;

    /**
     * Will move volumes that are in forwarding state to
     * finish forwarding state -- forwarding will actually end when
     * all updates that are currently queued are processed.
     */
    Error notifyDMTClose(int64_t dmtVersion);
    void finishForwarding(fds_volid_t volid);

    /**
     * A callback from stats collector to sample DM-specific stats
     */
    void sampleDMStats(fds_uint64_t timestamp);

    /**
     * A callback from stats collector with stats for a given volume
     * to add to the aggregator
     */
    void handleLocalStatStream(fds_uint64_t start_timestamp,
                               fds_volid_t volume_id,
                               const std::vector<StatSlot>& slots);

    void setup_metasync_service();

    explicit DataMgr(CommonModuleProviderIf *modProvider);
    ~DataMgr();
    std::map<fds_io_op_t, dm::Handler*> handlers;
    dmQosCtrl   *qosCtrl;

    fds_uint32_t qosThreadCount = 10;
    fds_uint32_t qosOutstandingTasks = 20;

    // Test related members
    fds_bool_t testUturnAll;
    fds_bool_t testUturnUpdateCat;
    fds_bool_t testUturnStartTx;
    fds_bool_t testUturnSetMeta;

    /* Overrides from Module */
    virtual int  mod_init(SysParams const *const param) override;
    virtual void mod_startup() override;
    virtual void mod_shutdown() override;
    virtual void mod_enable_service() override;

    int run();

    void swapMgrId(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg);
    void setResponseError(fpi::FDSP_MsgHdrTypePtr& msg_hdr, const Error& err);

    std::string getPrefix() const;
    fds_bool_t volExists(fds_volid_t vol_uuid) const;

    /* TODO(Rao): Add the new refactored DM messages handlers here */
    void updateCatalog(DmRequest *io);
    /* End of new refactored DM message handlers */

    void flushIO();
    void scheduleDeleteCatObjSvc(void * _io);
    void setBlobMetaDataBackend(const DmRequest *request);
    void snapVolCat(DmRequest *io);
    void handleDMTClose(DmRequest *io);
    void handleForwardComplete(DmRequest *io);
    void handleStatStream(DmRequest *io);
    void handleDmFunctor(DmRequest *io);

    Error processVolSyncState(fds_volid_t volume_id, fds_bool_t fwd_complete);

    /**
     * Timeout to send DMT close ack if not sent yet and stop forwarding
     * cat updates for volumes that are still in 'finishing forwarding' state
     */
    void finishCloseDMT();

    /**
     * Create snapshot of a specified volume
     */
    Error createSnapshot(const fpi::Snapshot& snapDetails);

    /**
     * Delete snapshot
     */
    Error deleteSnapshot(const fds_uint64_t snapshotId);

    virtual std::string getSnapDirBase() const override;

    Error deleteVolumeContents(fds_volid_t volId);

    virtual std::string getSysVolumeName(const fds_volid_t &volId) const override;

    virtual std::string getSnapDirName(const fds_volid_t &volId,
                                       const fds_volid_t snapId) const override;
    /**
     * Deletes unowned volumes.
     */
    void deleteUnownedVolumes();

    ///
    /// Cleanly shut down.
    ///
    /// run() will exit after this is called.
    ///
    void shutdown();

    // Runs sync protocol
    void runSyncProtocol(int32_t version,
                         const VolumeDesc &volDesc);
    /*
     * Gets and sets Number of primary DMs.
     */
    inline fds_uint32_t getNumOfPrimary()  {
        return (_numOfPrimary);
    }
    inline void setNumOfPrimary(fds_uint32_t num) {
        fds_verify(num > 0);
        _numOfPrimary = num;
    }
    inline FDS_QoSControl* getQosCtrl() const { return qosCtrl; }

    /**
     * Migration mgr for managing DM migrations
     */
    std::unique_ptr<DmMigrationMgr> dmMigrationMgr;

    friend class DMSvcHandler;
    friend class dm::GetBucketHandler;
    friend class dm::DmSysStatsHandler;
    friend class dm::DeleteBlobHandler;

private:
    ///
    /// Don't shut down until this gate is opened.
    ///
    util::ExecutionGate _shutdownGate;

    /**
     * Implementation of amIPrimary
     */
    fds_bool_t _amIPrimaryImpl(fds_volid_t &volUuid, bool topPrimary);

    /*
     * Number of primary DMs
     */
    fds_uint32_t _numOfPrimary;

    // Variables to track how frequently we call the diskCapacity checks
    fds_uint8_t sampleCounter;
    float_t lastCapacityMessageSentAt;

    /**
     * Send event message to OM
     */
    void sendEventMessageToOM(fpi::EventType eventType,
                              fpi::EventCategory eventCategory,
                              fpi::EventSeverity eventSeverity,
                              fpi::EventState eventState,
                              const std::string& messageKey,
                              std::vector<fpi::MessageArgs> messageArgs,
                              const std::string& messageFormat);

    // Send health check message
    void sendHealthCheckMsgToOM(fpi::HealthState serviceState,
                                fds_errno_t statusCode,
                                const std::string& statusInfo);


};

class CloseDMTTimerTask : public FdsTimerTask {
  public:
    typedef std::function<void ()> cbType;

    CloseDMTTimerTask(FdsTimer& timer, cbType cb)  //NOLINT
            : FdsTimerTask(timer), timeout_cb(cb) {
    }
    ~CloseDMTTimerTask() {}

    void runTimerTask();

  private:
    cbType timeout_cb;
};

namespace dmutil {
// location of volume
std::string getVolumeDir(const FdsRootDir* root,
                         fds_volid_t volId, fds_volid_t snapId = invalid_vol_id);

// location of all snapshots for a volume
std::string getSnapshotDir(const FdsRootDir* root, fds_volid_t volId);
std::string getVolumeMetaDir(const FdsRootDir* root, fds_volid_t volId);
std::string getLevelDBFile(const FdsRootDir* root, fds_volid_t volId, fds_volid_t snapId = invalid_vol_id);

/**
* @brief Returns list of volume id in dm catalog under FdsRootDir root
*
* @param root
* @param vecVolumes
*/
void getVolumeIds(const FdsRootDir* root, std::vector<fds_volid_t>& vecVolumes);

std::string getTimelineDBPath(const FdsRootDir* root);
std::string getExpungeDBPath(const FdsRootDir* root);
}  // namespace dmutil

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DATAMGR_H_

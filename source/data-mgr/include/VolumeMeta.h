/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Volume metadata class. Describes the per-volume metada
 * that is maintained by a data manager.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_

#include <string>
#include <map>
#include <fds_types.h>
#include <fds_error.h>
#include <util/Log.h>

#include <concurrency/Mutex.h>
#include <fds_volume.h>
#include <DmMigrationDest.h>
#include <DmMigrationSrc.h>
#include <VolumeInitializer.h>

namespace fds {

struct EPSvcRequest;

using DmMigrationSrcMap = std::map<NodeUuid, DmMigrationSrc::shared_ptr>;
using migrationCb = std::function<void(const Error& e)>;
using migrationSrcDoneCb = std::function<void(fds_volid_t volId, const Error &error)>;
using migrationDestDoneCb = std::function<void (NodeUuid srcNodeUuid,
							                    fds_volid_t volumeId,
							                    const Error& error)>;

struct VolumeMeta : HasLogger,  HasModuleProvider {
 public:
    /**
     * volume  meta forwarding state
     */
    typedef enum {
        VFORWARD_STATE_NONE,           // not fowarding
        VFORWARD_STATE_INPROG,         // forwarding
        VFORWARD_STATE_FINISHING       // forwarding commits of open trans
    } fwdStateType;

    /*
     * Default constructor should NOT be called
     * directly. It is needed for STL data struct
     * support.
     */
    VolumeMeta();
    VolumeMeta(CommonModuleProviderIf *modProvider,
               const std::string& _name,
               fds_volid_t _uuid,
               fds_log* _dm_log,
               VolumeDesc *v_desc,
               DataMgr *_dm);
    ~VolumeMeta();
    void setSequenceId(sequence_id_t seq_id);
    sequence_id_t getSequenceId();
    inline void setOpId(const int64_t &id) { opId = id; }
    inline void incrementOpId() { ++opId; }
    inline const int64_t& getOpId() const { return opId; }
    inline fpi::ResourceState getState() const { return vol_desc->state; }
    void setState(const fpi::ResourceState &state, const std::string &logCtx);
    inline bool isActive() const { return vol_desc->state == fpi::Active; }
    inline bool isSyncing() const { return vol_desc->state == fpi::Syncing; }
    inline int64_t getId() const { return vol_desc->volUUID.get(); }
    inline int32_t getVersion() const { return version; }
    inline void setVersion(int32_t version) { this->version = version; }
    inline fpi::SvcUuid getCoordinatorId() const { return vol_desc->getCoordinatorId(); }
    inline bool isCoordinatorSet() const { return vol_desc->isCoordinatorSet(); }
    std::string logString() const;

    static inline bool isReplayOp(const std::string &payloadHdr) {
        return payloadHdr == "replay"; 
    }

    void dmCopyVolumeDesc(VolumeDesc *v_desc, VolumeDesc *pVol);

    std::function<void(EPSvcRequest*,const Error &e, StringPtr)>
    makeSynchronized(const std::function<void(EPSvcRequest*,const Error &e, StringPtr)> &f);

    StatusCb makeSynchronized(const StatusCb &f);
    BufferReplay::ProgressCb synchronizedProgressCb(const BufferReplay::ProgressCb &f);


    /**
     * DM Migration related
     */
    Error startMigration(NodeUuid& srcDmUuid,
                         fpi::FDSP_VolumeDescType &vol,
                         migrationCb doneCb);

    Error serveMigration(DmRequest *dmRequest);

    /**
     * Returns true if DM is forwarding this volume's
     * committed updates
     */
    fds_bool_t isForwarding() const {
        return (fwd_state != VFORWARD_STATE_NONE);
    }
    /**
     * Returns true if DM is forwarding this volume's
     * committed updates after DMT close -- those are commits
     * of transactions that were open at the time of DMT close
     */
    fds_bool_t isForwardFinishing() const {
        return (fwd_state == VFORWARD_STATE_FINISHING);
    }
    void setForwardInProgress() {
        fwd_state = VFORWARD_STATE_INPROG;
    }
    void setForwardFinish() {
        fwd_state = VFORWARD_STATE_NONE;
    }
    /**
     * If state is 'in progress', will move to 'finishing'
     */
    void finishForwarding();


    VolumeDesc *vol_desc;


    /*
     * per volume queue
     */
    boost::intrusive_ptr<FDS_VolumeQueue>  dmVolQueue;
    /* For runinng the sync protocol */
    VolumeInitializerPtr                   initializer;

 private:
    /*
     * This class is non-copyable.
     * Disable copy constructor/assignment operator.
     */
    VolumeMeta(const VolumeMeta& _vm);
    VolumeMeta& operator= (const VolumeMeta rhs);

    fds_mutex  *vol_mtx;

    /**
     * volume meta forwarding state
     */
    fwdStateType fwd_state;  // write protected by vol_mtx

    /**
     * latest sequence ID is not part of the volume descriptor because it will
     * always be out of date. stashing it here to provide to the AM on attach.
     * value updated on sucessful requests, but operations with later sequence
     * ids may be still buffered somewhere in DM.
     */
    sequence_id_t sequence_id;
    fds_mutex sequence_lock;
    /* Operation id. Every update/commit operation against volume should have this id.
     * This id should be sequential.
     */
    int64_t opId;
    /* Version of the volume.  As volume goes up/down this incremented.  This id
     * persisted to disk as well
     */
    int32_t version;

    /**
     * This volume could be a source of migration to multiple nodes.
     */
    DmMigrationSrcMap migrationSrcMap;
    fds_rwlock migrationSrcMapLock;

    /**
     * This volume can only be a destination to one node
     */
    DmMigrationDest::unique_ptr migrationDest;

    /**
     * DataMgr reference
     */
    DataMgr *dataManager;

    /**
     * Internally create a source and runs it
     */
    Error createMigrationSource(NodeUuid destDmUuid,
                                const NodeUuid &mySvcUuid,
                                fpi::CtrlNotifyInitialBlobFilterSetMsgPtr filterSet,
                                migrationCb cleanup);

    /**
     * Internally cleans up a source
     */
    void cleanUpMigrationSource(fds_volid_t volId,
                                const Error &err,
                                const NodeUuid destDmUuid);

    /**
     * Internally cleans up the destination
     */
    void cleanUpMigrationDestination(NodeUuid srcNodeUuid,
                                     fds_volid_t volId,
                                     const Error &err);

    /**
     * Stores the hook for Callback to the volume group manager
     */
    migrationCb cbToVGMgr;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_

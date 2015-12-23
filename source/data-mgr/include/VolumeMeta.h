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

namespace fds {

#if 0
// TODO(Rao): Enable when ready
/**
* @brief Holds syncing related data
*/
struct SyncContext {
    SyncContext()
    : bufferIo(false),
    startingBufferOpId(0)
    {
    }
    fpi::SvcUuid                            syncPeer;
    bool                                    bufferIo;
    int64_t                                 startingBufferOpId; 
    /* Active IO that's been buffered while active transaction are copied from the sync peer */
    std::list<VolumeIoBasePtr>              bufferedIo;
    /* Commits that are buffered during sync */
    // VolumeCommitLog                         bufferCommitLog;
};
using SyncContextPtr = std::unique_ptr<SyncContext>;
#endif

class VolumeMeta : public HasLogger {
 public:
    /**
     * volume  meta forwarding state
     */
    typedef enum {
        VFORWARD_STATE_NONE,           // not fowarding
        VFORWARD_STATE_INPROG,         // forwarding
        VFORWARD_STATE_FINISHING       // forwarding commits of open trans
    } fwdStateType;

  private:
    fds_mutex  *vol_mtx;

    /**
     * volume meta forwarding state
     */
    fwdStateType fwd_state;  // write protected by vol_mtx

    /*
     * This class is non-copyable.
     * Disable copy constructor/assignment operator.
     */
    VolumeMeta(const VolumeMeta& _vm);
    VolumeMeta& operator= (const VolumeMeta rhs);

 public:
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

    /**
     * latest sequence ID is not part of the volume descriptor because it will
     * always be out of date. stashing it here to provide to the AM on attach.
     * value updated on sucessful requests, but operations with later sequence
     * ids may be still buffered somewhere in DM.
     */
 private:
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
 public:
    /*
     * Default constructor should NOT be called
     * directly. It is needed for STL data struct
     * support.
     */
    VolumeMeta();
    VolumeMeta(const std::string& _name,
               fds_volid_t _uuid,
               VolumeDesc *v_desc);
    VolumeMeta(const std::string& _name,
               fds_volid_t _uuid,
               fds_log* _dm_log,
               VolumeDesc *v_desc);
    ~VolumeMeta();
    void setSequenceId(sequence_id_t seq_id);
    sequence_id_t getSequenceId();
    inline void setOpId(const int64_t &id) { opId = id; }
    inline const int64_t& getOpId() const { return opId; }
    inline fpi::ResourceState getState() const { return vol_desc->state; }
    inline void setState(const fpi::ResourceState &state) { vol_desc->state = state; }
    inline bool isActive() const { return vol_desc->state == fpi::Active; }
    inline bool isSyncing() const { return vol_desc->state == fpi::Syncing; }
    inline int64_t getId() const { return vol_desc->volUUID.get(); }
    inline int32_t getVersion() const { return version; }
    inline fpi::SvcUuid getCoordinatorId() const { return vol_desc->getCoordinatorId(); }


    void dmCopyVolumeDesc(VolumeDesc *v_desc, VolumeDesc *pVol);
    /*
     * per volume queue
     */
    boost::intrusive_ptr<FDS_VolumeQueue>  dmVolQueue;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_

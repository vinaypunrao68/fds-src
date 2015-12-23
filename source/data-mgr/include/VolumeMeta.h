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

namespace fds {

using DmMigrationSrcMap = std::map<NodeUuid, DmMigrationSrc::shared_ptr>;
using migrationCb = std::function<void(const Error& e)>;
using migrationSrcDoneCb = std::function<void(fds_volid_t volId, const Error &error)>;
using migrationDestDoneCb = std::function<void (NodeUuid srcNodeUuid,
							                    fds_volid_t volumeId,
							                    const Error& error)>;


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
 public:
    void setSequenceId(sequence_id_t seq_id);
    sequence_id_t getSequenceId();

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
               VolumeDesc *v_desc,
               DataMgr *_dm);
    ~VolumeMeta();

    void dmCopyVolumeDesc(VolumeDesc *v_desc, VolumeDesc *pVol);
    /*
     * per volume queue
     */
    boost::intrusive_ptr<FDS_VolumeQueue>  dmVolQueue;

    /**
     * DM Migration related
     */
 public:
    Error startMigration(NodeUuid& srcDmUuid,
                         fpi::FDSP_VolumeDescType &vol,
                         int64_t migrationId,
                         migrationCb doneCb);

    Error ServeMigration(DmRequest *dmRequest);

 private:
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
     * DataMgr ptr
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
                                const NodeUuid destDmUuid,
                                int64_t migrationid);

    /**
     * Internally cleans up the destination
     */
    void cleanUpMigrationDestination(NodeUuid srcNodeUuid,
                                     fds_volid_t volId,
                                     const Error &err,
                                     int64_t migrationid);

    /**
     * Stores the hook for Callback to the volume group manager
     */
    migrationCb cbToVGMgr;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_

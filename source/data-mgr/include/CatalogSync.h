/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_
#define SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_

#include <atomic>
#include <unordered_map>
#include <string>
#include <set>

#include <fds_error.h>
#include <fds_types.h>
#include <fds_module.h>
#include <util/Log.h>
#include <fds_config.hpp>
#include <fds_process.h>
#include <concurrency/Mutex.h>
#include <DmIoReq.h>
#include <DmBlobTypes.h>
#include <fdsp/svc_types_types.h>
#include <net/PlatNetSvcHandler.h>
#include <net/SvcRequest.h>
#include <fdsp/DMSvc.h>

namespace fpi = FDS_ProtocolInterface;

namespace fds {

    struct DataMgr;

     class CatalogSyncMgr;

     // Callback to DM svc handler for any DMT migration events
     // DMT update, DMT close and 
     typedef std::function<void (const Error&)> OmDMTMsgCbType;

     typedef enum {
         CATSYNC_INITIAL_SYNC_DONE = 0,  // finished initial rsync
         CATSYNC_DELTA_SYNC_DONE         // finished second (delta) rsync
     } catsync_notify_evt_t;

    /**
     * Callback type to notify that catalog sync process is finished for
     * a given node
     */
     typedef std::function<void (catsync_notify_evt_t event,
                                 fds_volid_t vol_id,
                                 const Error& error)> catsync_done_handler_t;

    /**
     * Manages per-volume catalog sync job
     * Process is as follows:
     *    1) startSync() puts CatalogSync into CSSTATE_INITIAL_SYNC state and
     *       puts sync work items into qos queue, one per volume
     *    2) When sync work item is pulled from Qos queue, snapDoneCb is called,
     *       progress is recorded. If this is the last snapDoneCb (we finished
     *       all volumes), CatalogSync sets state to CSTATE_DELTA_SYNC and
     *       notifies CatalogSyncMgr
     *    3) performDeltaSync() must be called only when CatalogSync is in
     *       CSSTATE_DELTA_SYNC (we finished initial sync for all volumes).
     *       CatalogSync puts sync work items into qos queue, one per volume
     *       (we save the volumes we started rsync for that were
     *       passed with startSync()).
     *    4) When sync work item is pulled from QoS queue, snapDoneCb is called
     *       with flag that this is the second rsync; progress is recorded. If
     *       we done doing delta rsync for all the volumes for which CatalogSync
     *       is responsible, we set flag to CSTATE_FORWARDING and notify
     *       CatalogSyncMgr that we are done with delta sync
     */
    class CatalogSync {
  public:
        CatalogSync(const NodeUuid &uuid,
                    DmIoReqHandler* dm_req_hdlr);
        ~CatalogSync();

        typedef enum {
            CSSTATE_READY = 0,      // we can call startSync
            CSSTATE_INITIAL_SYNC,   // initial sync in progress
            CSSTATE_DELTA_SYNC ,    // second (delta) sync in progress
            CSSTATE_FORWARD_ONLY,   // all rsyncs done, only forwarding updates
            CSSTATE_DONE,            // done syncing
            CSSTATE_ABORT           // aborting the sync
        } csStateType;

        /**
         * Actually start catalog sync process for given set of
         * volumes to node for which this CatalogSync is responsible for.
         * @param[in] done_evt_hdlr a callback CatalogSync will call
         * when initial sync and delta sync is finished.
         * @param[in] volumes is set of volumes which this CatalogSync
         * will need to sync; on function return, the set will be empty
         */
        Error startSync(std::set<fds_volid_t>* volumes,
                        catsync_done_handler_t done_evt_hdlr);

        /**
         * Start process for performing delta syncs for all the volumes
         * we did initial sync (passed with startSync()).
         */
        void doDeltaSync();

        /**
         * Forward catalog update to DM
         * TODO(xxx) add parameters (DmRequest?)
         * CatalogSync must be responsible for volume 'volid'
         * CatalogSync must be in CSSTATE_FORWARDING state
         * @return ERR_OK on success; or networks error
         */
        Error forwardCatalogUpdate(DmIoCommitBlobTx *commitBlobReq,
                                   blob_version_t blob_version,
                                   const BlobObjList::const_ptr& blob_obj_list,
                                   const MetaDataList::const_ptr& meta_list);

        void fwdCatalogUpdateMsgResp(DmIoCommitBlobTx *commitReq,
                                     EPSvcRequest* req,
                                     const Error& error,
                                     boost::shared_ptr<std::string> payload);

        Error issueVolSyncStateMsg(fds_volid_t volId,
                                   fds_bool_t foward_complete);

        /**
         * @return true if catalog sync is finished
         */
        fds_bool_t isInitialSyncDone() const;
        fds_bool_t isDeltaSyncDone() const;
        fds_bool_t syncDone();

        /**
         * Callback from data mgr that volume cat initial push
         * is complete
         */
        void snapDoneCb(fds_volid_t volid,
                        const Error& error
                        /*vol_snap_req*/
                        /* open file desc ?*/);

        /**
         * Callback from data mgr that volume cat delta push
         * is complete
         */
        void deltaDoneCb(fds_volid_t volid,
                         const Error& error
                         /*vol_snap_req*/
                         /* open file desc ?*/);

        /**
         * Notification that we finished sync/forwarding for
         * volume 'volid', will remove vol from list
         */
        void handleVolumeDone(fds_volid_t volid);

        /**
         * @return true if CatalogSync is responsible for syncing
         * given volume 'volid'
         */
        inline fds_bool_t hasVolume(fds_volid_t volid) {
            return (sync_volumes.count(volid) > 0);
        }

        inline fds_bool_t emptyVolume() {
            return (sync_volumes.empty());
        }


        void abortMigration(Error err);

  private:  // methods
        Error sendMetaSyncDone(fds_volid_t volid, fds_bool_t forward_done);

        /**
         * Record progress of one volume (either initial sync
         * where expected state is CSSTATE_INITIAL_SYNC or delta sync
         * where expected state is CSSTATE_DELTA_SYNC)
         * @return total volumes done
         */
        fds_uint32_t recordVolSyncDone(csStateType expected_state);

  private:
        fpi::SvcUuid svc_uuid;
        NodeUuid node_uuid;  // destination node

        std::atomic<csStateType> state;  // current state

        /**
         * Pointer to DmIoReqHandler so we can queue work/IO to
         * QoS queues; passed in constructor, does not own.
         */
        DmIoReqHandler *dm_req_handler;

        /**
         * This callback is passed via startSync(). Will call
         * when catalog sync job is finished
         */
        catsync_done_handler_t done_evt_handler;

        /**
         * Cashed volumes we are working with
         */
        std::set<fds_volid_t> sync_volumes;

        /**
         * For tracking sync progress
         * vols_done are used for both initial sync and delta sync
         */
        std::atomic<fds_uint32_t> vols_done;  // num of vols synced
    };

    typedef boost::shared_ptr<CatalogSync> CatalogSyncPtr;
    typedef std::unordered_map<NodeUuid, CatalogSyncPtr, UuidHash> CatSyncMap;

    /**
     * Manages Catalog sync process
     */
    class CatalogSyncMgr: public Module {
  public:
        CatalogSyncMgr(fds_uint32_t max_jobs,
                       DmIoReqHandler* dm_req_hdlr,
                       DataMgr& dataManager);
        virtual ~CatalogSyncMgr();

        /* Overrides from Module */
        virtual void mod_startup() override;
        virtual void mod_shutdown() override;

        /**
         * Called when DM received push meta message from OM
         * to start catalog sync process for list of vols and to which
         * nodes to sync.
         * For now we will not allow to start catalog sync if the previous
         * cat sync is still in progress, later we may revisit this...
         */
        Error startCatalogSync(const fpi::FDSP_metaDataList& metaVol,
                               OmDMTMsgCbType cb);

        /**
         * Forward catalog update to DM to which we are pushing vol meta for the
         * corresponding volume.
         * Must be called only for volumes for which sync is in progress
         */
        Error forwardCatalogUpdate(DmIoCommitBlobTx *commitBlobReq,
                                   blob_version_t blob_version,
                                   const BlobObjList::const_ptr& blob_obj_list,
                                   const MetaDataList::const_ptr& meta_list);

        /**
         * Notifies destination DM to start servicing IO for volume 'volid'
         */
        Error issueServiceVolumeMsg(fds_volid_t volid);

        /**
         * Called when forwarding can be finished for volume 'volid'
         * so volume-related data structures can be freed
         * @return true if all volumes finished forwarding metadata
         */
        fds_bool_t finishedForwardVolmeta(fds_volid_t volid);

        /**
         * @return true if sync is in progress = time between push meta
         * is received from OM and time when we finished forwarding
         * cat updates for all volumes
         */
        inline fds_bool_t isSyncInProgress() const { return sync_in_progress; }

        /**
         * Sets time when DM received DMT close from OM to now
         */
        void setDmtCloseNow();
        inline fds_uint64_t dmtCloseTs() const { return dmtclose_ts; }

        /**
         * Callback from CatalogSync that sync is finished for given volume
         */
        void syncDoneCb(catsync_notify_evt_t event,
                        fds_volid_t volid,
                        const Error& error);

        /**
        * Aborts sync when error state is reached in DMT state machine
        */
        Error abortMigration();


  private:
        DataMgr& dataManager_;

        fds_bool_t sync_in_progress;

        /**
         * Pointer to DmIoReqHandler so we can queue work/IO to
         * QoS queues; passed in constructor, does not own.
         */
        DmIoReqHandler *dm_req_handler;

        /**
         * Volume id to Catalog Sync object which are already doing
         * sync job or are scheduled to perform sync job
         */
        CatSyncMap cat_sync_map;
        fds_mutex cat_sync_lock;  // protects catSyncMap and sync_in_progress

        /**
         * Timestamp when DM received DMT close
         */
        fds_uint64_t dmtclose_ts;

        // callback to svc handler to send ack for DMT update
        OmDMTMsgCbType omDmtUpdateCb;
        OmDMTMsgCbType omPushMetaCb;
    };

    typedef boost::shared_ptr<CatalogSyncMgr> CatalogSyncMgrPtr;

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_

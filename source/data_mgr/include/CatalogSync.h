/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_
#define SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_

#include <atomic>
#include <unordered_map>
#include <string>
#include <set>

#include <dm-platform.h>
#include <fds_error.h>
#include <fds_types.h>
#include <fds_module.h>
#include <util/Log.h>
#include <fds_config.hpp>
#include <fds_counters.h>
#include <fds_process.h>
#include <fdsp/FDSP_MetaSyncReq.h>
#include <fdsp/FDSP_MetaSyncResp.h>
#include <concurrency/Mutex.h>
#include <fdsp/FDSP_types.h>
#include <DmIoReq.h>
#include <NetSession.h>

namespace fpi = FDS_ProtocolInterface;

namespace fds {


     class FDSP_MetaSyncRpc;  // forward declaration
     class OMgrClient;
     class CatalogSyncMgr;

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
                                 OMgrClient* omclient,
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
                    OMgrClient* omclient,
                    DmIoReqHandler* dm_req_hdlr);
        ~CatalogSync();

        typedef enum {
            CSSTATE_READY = 0,      // we can call startSync
            CSSTATE_INITIAL_SYNC,   // initial sync in progress
            CSSTATE_DELTA_SYNC ,    // second (delta) sync in progress
            CSSTATE_FORWARDING,     // forwarding updates
            CSSTATE_DONE            // done syncing
        } csStateType;

        /**
         * Actually start catalog sync process for given set of
         * volumes to node for which this CatalogSync is reponsible for.
         * @param[in] done_evt_hdlr a callback CatalogSync will call
         * when initial sync and delta sync is finished.
         * @param[in] volumes is set of volumes which this CatalogSync
         * will need to sync; on function return, the set will be empty
         */
        Error startSync(std::set<fds_volid_t>* volumes,
                        netMetaSyncClientSession* client,
                        catsync_done_handler_t done_evt_hdlr);

        /**
         * Start process for performing delta syncs for all the volumes
         * we did initial sync (passed with startSync()).
         */
        void doDeltaSync();

        /**
         * Forward catalog update to DM
         * TODO(xxx) add parameters (dmCatReq?)
         * CatalogSync must be responsible for volume 'volid'
         * CatalogSync must be in CSSTATE_FORWARDING state
         * @return ERR_OK on success; or networks error
         */
        Error forwardCatalogUpdate(dmCatReq  *updCatReq);

        /**
         * @return true if catalog sync is finished
         */
        fds_bool_t isInitialSyncDone() const;
        fds_bool_t isDeltaSyncDone() const;

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
         * @return true if CatalogSync is reponsible for syncing
         * given volume 'volid'
         */
        inline fds_bool_t hasVolume(fds_volid_t volid) {
            return (sync_volumes.count(volid) > 0);
        }

  private:  // methods
        Error sendMetaSyncDone(fds_volid_t volid);

        /**
         * Record progress of one volume (either initial sync
         * where expected state is CSSTATE_INITIAL_SYNC or delta sync
         * where expected state is CSSTATE_DELTA_SYNC)
         * @return total volumes done
         */
        fds_uint32_t recordVolumeDone(csStateType expected_state);

  private:
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
         * Cashed OM client passed via constructor, does not own
         */
        OMgrClient* om_client;

        /**
         * Cashed volumes we are working with
         */
        std::set<fds_volid_t> sync_volumes;

        /**
         * For tracking sync progress
         * vols_done are used for both initial sync and delta sync
         */
        std::atomic<fds_uint32_t> vols_done;  // num of vols synced

        /**
         * Client to destination DM with uuid 'node_uuid'
         */
        netMetaSyncClientSession *meta_client;
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
                       netSessionTblPtr netSession);
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
                               OMgrClient* omclient,
                               const std::string& context);

        /**
         * Called when DM receives DMT commit so that in-progress catalog syncs
         * can do delta rsync to DMs.
         * @return ERR_NOT_READY if called in the middle of initial rsync; returns
         * ERR_OK if success.
         */
        Error startCatalogSyncDelta(const std::string& context);

        /**
         * Called when OM sends DMT close message to DM to notify that we are done
         * catalog sync and forwarding, etc, so we can cleanup state of this sync
         */
        void notifyCatalogSyncFinish();

        /**
         * Forward catalog update to DM to which we are pushing vol meta for the
         * corresponding volume.
         * Must be called only for volumes for which sync is in progress
         * TODO(xxx) add parameters (dmCatReq?)
         */
        Error forwardCatalogUpdate(dmCatReq  *updCatReq);

        fds_bool_t isSyncInProgress() const { return sync_in_progress; }

        /**
         * Callback from CatalogSync that sync is finished for given volume
         */
        void syncDoneCb(catsync_notify_evt_t event,
                        fds_volid_t volid,
                        OMgrClient* omclient,
                        const Error& error);

  private:  // methods
        netMetaSyncClientSession*
                create_metaSync_client(const NodeUuid& node_uuid, OMgrClient* omclient);

  private:
        fds_bool_t sync_in_progress;
        /**
         * max number of volume sync jobs we can have in progress
         * at the same time.
         */
        fds_uint32_t max_sync_inprogress;

        /**
         * Pointer to DmIoReqHandler so we can queue work/IO to
         * QoS queues; passed in constructor, does not own.
         */
        DmIoReqHandler *dm_req_handler;

        /**
         * Volume id to Catalog Sync object which are already doing
         * sync job or are scheduled to perform sync job
         */
        std::string cat_sync_context;  // to return when sync is done
        CatSyncMap cat_sync_map;
        fds_mutex cat_sync_lock;  // protects catSyncMap and sync_in_progress

        /* Net session  handlers */
        netSessionTblPtr netSessionTbl;
        boost::shared_ptr<FDSP_MetaSyncRpc> meta_handler;
        netMetaSyncServerSession *meta_session;
    };

    typedef boost::shared_ptr<CatalogSyncMgr> CatalogSyncMgrPtr;


    class FDSP_MetaSyncRpc : virtual public fpi::FDSP_MetaSyncReqIf,
            virtual public fpi::FDSP_MetaSyncRespIf, public HasLogger
    {
   public:
        FDSP_MetaSyncRpc(CatalogSyncMgr &meta_sync_, fds_log *log)
            : metaSyncMgr(meta_sync_) {
            SetLog(log);
        }
        ~FDSP_MetaSyncRpc() {}

        std::string log_string() {
            return "FDSP_MigrationPathRpc";
        }
        void PushMetaSyncReq(const fpi::FDSP_MsgHdrType& fdsp_msg,
                             const fpi::FDSP_UpdateCatalogType& push_meta_req) {
            // Don't do anything here. This stub is just to keep cpp compiler happy
        }
        void PushMetaSyncReq(fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                             fpi::FDSP_UpdateCatalogTypePtr& push_meta_req);

        void MetaSyncDone(const fpi::FDSP_MsgHdrType& fdsp_msg,
                          const fpi::FDSP_VolMetaState& vol_meta) {
            // Don't do anything here. This stub is just to keep cpp compiler happy
        }
        void MetaSyncDone(fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                          fpi::FDSP_VolMetaStatePtr& vol_meta);

        void PushMetaSyncResp(const fpi::FDSP_MsgHdrType& fdsp_msg,
                              const fpi::FDSP_UpdateCatalogType& push_meta_resp) {
            // Don't do anything here. This stub is just to keep cpp compiler happy
        }
        void PushMetaSyncResp(fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                              fpi::FDSP_UpdateCatalogTypePtr& push_meta_resp) {
        }
        void MetaSyncDoneResp(const fpi::FDSP_MsgHdrType& fdsp_msg,
                              const fpi::FDSP_VolMetaState& vol_meta) {
            // Don't do anything here. This stub is just to keep cpp compiler happy
        }
        void MetaSyncDoneResp(fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                              fpi::FDSP_VolMetaStatePtr& vol_meta) {
        }
  protected:
        CatalogSyncMgr  &metaSyncMgr;
    };
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_

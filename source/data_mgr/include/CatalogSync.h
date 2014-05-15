/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_
#define SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_

#include <atomic>
#include <unordered_map>
#include <string>

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
using  namespace  ::FDS_ProtocolInterface;  // NOLINT

namespace fds {


     class FDSP_MetaSyncRpc;  // forward declaration
     class OMgrClient;
     class CatalogSyncMgr;

    /**
     * Callback type to notify that catalog sync process is finished for
     * a given volume
     */
     typedef std::function<void (fds_volid_t vol_id,
                                 OMgrClient* omclient,
                                 const Error& error)> catsync_done_handler_t;

    /**
     * Manages per-volume catalog sync job
     */
    class CatalogSync {
  public:
        CatalogSync(fds_volid_t vol_id,
                    const NodeUuid &uuid,
                    DmIoReqHandler* dm_req_hdlr);
        ~CatalogSync();

        typedef enum {
            CSSTATE_READY = 0,     // we can call startSync
            CSSTATE_IN_PROGRESS,   // sync in progress
            CSSTATE_DONE           // done syncing
        } csStateType;

        /**
         * Actually start catalog sync process. Will queue
         * work item to make a level db snapshot
         * @param[in] done_evt_hdlr a callback CatalogSync will call
         * when the sync process is finished.
         */
        Error startSync(catsync_done_handler_t done_evt_hdlr);

        /**
         * @return true if catalog sync is finished
         */
        inline fds_bool_t isDone() const {
            return (state == CSSTATE_DONE);
        }

        /**
         * Callback from data mgr that volume cat snapshot
         * is complete; can do rsync here
         */
        void snapDoneCb(const Error& error,
                        OMgrClient* omclient
                        /*vol_snap_req*/
                        /* open file desc ?*/);

  private:
        fds_volid_t volume_id;  // volume to sync
        NodeUuid node_uuid;  // destination node

        csStateType state;  // current state

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
    };

    typedef boost::shared_ptr<CatalogSync> CatalogSyncPtr;
    typedef std::unordered_map<fds_volid_t, CatalogSyncPtr> CatSyncMap;

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
                               const std::string& context);

        /**
         * Called when OM sends DMT close message to DM to notify that we are done
         * catalog sync and forwarding, etc, so we can cleanup state of this sync
         */
        void notifyCatalogSyncFinish();

        /**
         * Callback from CatalogSync that sync is finished for given volume
         */
        void syncDoneCb(fds_volid_t volid,
                        OMgrClient* omclient,
                        const Error& error);

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


    class FDSP_MetaSyncRpc : virtual public FDSP_MetaSyncReqIf,
        virtual public FDSP_MetaSyncRespIf, public HasLogger
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
       void PushMetaSyncReq(const FDSP_MsgHdrType& fdsp_msg,
               const FDSP_UpdateCatalogType& push_meta_req) {
       }

       void PushMetaSyncReq(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
               boost::shared_ptr<FDSP_UpdateCatalogType>& push_meta_req) {
          // Don't do anything here. This stub is just to keep cpp compiler happy
       }
       void MetaSyncDone(const FDSP_MsgHdrType& fdsp_msg,
                       const FDSP_VolMetaState& vol_meta) {
       }
       void MetaSyncDone(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                        boost::shared_ptr<FDSP_VolMetaState>& vol_meta) {
          // Don't do anything here. This stub is just to keep cpp compiler happy
       }
       void PushMetaSyncResp(const FDSP_MsgHdrType& fdsp_msg,
                       const FDSP_UpdateCatalogType& push_meta_resp) {
       }
       void PushMetaSyncResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                        boost::shared_ptr<FDSP_UpdateCatalogType>& push_meta_resp) {
          // Don't do anything here. This stub is just to keep cpp compiler happy
       }
       void MetaSyncDoneResp(const FDSP_MsgHdrType& fdsp_msg,
                       const FDSP_VolMetaState& vol_meta) {
       }
       virtual void MetaSyncDoneResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                       boost::shared_ptr<FDSP_VolMetaState>& vol_meta) {
          // Don't do anything here. This stub is just to keep cpp compiler happy
       }
    protected:
       CatalogSyncMgr  &metaSyncMgr;
    };
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_

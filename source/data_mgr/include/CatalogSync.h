/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_
#define SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_

#include <atomic>
#include <unordered_map>
#include <string>

#include <fds_error.h>
#include <fds_types.h>
#include <fdsp/FDSP_types.h>

namespace fds {

    /**
     * Callback type to notify that catalog sync process is finished for
     * a given volume
     */
    typedef std::function<void (fds_volid_t vol_id,
                                const error& error)> catsync_done_handler_t;

    /**
     * Manages per-volume catalog sync job
     */
    class CatalogSync {
  public:
        // TODO(xxx) add params for destination,
        // ip address? what else?
        CatalogSync(fds_volid_t vol_id,
                    DmIoReqHandler* dm_req_hdlr);
        ~CatalogSync();

        /**
         * Actually start catalog sync process. Will queue
         * work item to make a level db snapshot
         * @param[in] done_evt_hdlr a callback CatalogSync will call
         * when the sync process is finished.
         */
        Error startSync(catsync_done_handler_t done_evt_hdlr);

        /**
         * Callback from data mgr that volume cat snapshot
         * is complete; can do rsync here
         */
        void snapDoneCb(const Error& error
                        /*vol_snap_req*/
                        /* open file desc ?*/);

  private:
        fds_volid_t volume_id;  // volume to sync
        // TODO(xxx) where to sync: node uuid or ip?

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
    class CatalogSyncMgr {
  public:
        CatalogSyncMgr(fds_uint32_t max_jobs,
                       DmIoReqHandler* dm_req_hdlr,
                       OMgrClient* omc);
        ~CatalogSyncMgr();

        /**
         * Called when DM received push meta message from OM
         * to start catalog sync process for list of vols and to which
         * nodes to sync.
         * For now we will not allow to start catalog sync if the previous
         * cat sync is still in progress, later we may revisit this...
         */
        Error startCatalogSync(const FDSP_metaDataList& metaVol);

  private:
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
         * OMgrClient pointer passed down from DataMgr, so we can
         * resolve node uuid to 
         */
        OMgrClient *parent_omc;

        /**
         * Volume id to Catalog Sync object which are already doing
         * sync job or are scheduled to perform sync job
         */
        CatSyncMap cat_sync_map;
        fds_mutex cat_sync_lock;  // protects catSyncMap
    };

    typedef boost::shared_ptr<CatalogSyncMgr> CatalogSyncMgrPtr;

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_CATALOGSYNC_H_

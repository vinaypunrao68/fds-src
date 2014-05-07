/*                                                                                                                                          
 * Copyright 2014 Formation Data Systems, Inc.                                                                                              
 */

#include <util/Log.h>
#include <fds_assert.h>
#include <CatalogSync.h>

namespace fds {

/****** CatalogSync implementation ******/

CatalogSync::CatalogSync(fds_volid_t vol_id,
                         DmIoReqHandler* dm_req_hdlr)
        : volume_id(vol_id) {
}

CatalogSync::~CatalogSync() {
}

Error CatalogSync::startSync(catsync_done_handler_t done_evt_hdlr) {
    Error err(ERR_OK);
    // TODO(xxx) queue qos request to do db snapshot and do rsync
    return err;
}

void CatalogSync::snapDoneCb(const Error& error) {
}

/***** CatalogSyncManager implementation ******/

CatalogSyncMgr::CatalogSyncMgr(fds_uint32_t max_jobs,
                               DmIoReqHandler* dm_req_hdlr)
        : Module("CatalogSyncMgr"),
          max_sync_inprogress(max_jobs),
          dm_req_handler(dm_req_hdlr),
          cat_sync_lock("Catalog Sync lock") {
    LOGNORMAL << "Constructing CatalogSyncMgr";
}

CatalogSyncMgr::~CatalogSyncMgr() {
}

/**
 * Module start up code
 */
void CatalogSyncMgr::mod_startup()
{
    LOGNORMAL << "Will setup server for metadata path";
}

/**
 * Module shutdown code
 */
void CatalogSyncMgr::mod_shutdown()
{
    LOGNORMAL << "Ending metadata path sessions";
}

/**
 * Called when DM receives push meta message from OM to start catalog sync process for list
 * of vols and to which nodes to sync.
 */
Error
CatalogSyncMgr::startCatalogSync(const FDS_ProtocolInterface::FDSP_metaDataList& metaVol) {
    Error err(ERR_OK);

    return err;
}

}  // namespace fds

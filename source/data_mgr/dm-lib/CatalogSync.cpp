/*                                                                                                                                          
 * Copyright 2014 Formation Data Systems, Inc.                                                                                              
 */

#include <util/Log.h>
#include <fds_assert.h>
#include <fds_typedefs.h>
#include <CatalogSync.h>

namespace fds {

/****** CatalogSync implementation ******/

CatalogSync::CatalogSync(fds_volid_t vol_id,
                         const NodeUuid& uuid,
                         DmIoReqHandler* dm_req_hdlr)
        : volume_id(vol_id),
          node_uuid(uuid),
          dm_req_handler(dm_req_hdlr) {
}

CatalogSync::~CatalogSync() {
}

Error CatalogSync::startSync(catsync_done_handler_t done_evt_hdlr) {
    Error err(ERR_OK);
    LOGNORMAL << "Start sync for volume " << volume_id;

    // set callback to notify when sync job is done
    done_evt_handler = done_evt_hdlr;

    // queue qos request to do db snapshot and do rsync
    DmIoSnapVolCat* snap_req = new DmIoSnapVolCat();
    snap_req->io_type = FDS_DM_SNAP_VOLCAT;
    snap_req->node_uuid = node_uuid;
    snap_req->volId = volume_id;
    snap_req->dmio_snap_vcat_cb = std::bind(
        &CatalogSync::snapDoneCb, this, std::placeholders::_1);

    // enqueue to qos queue
    // TODO(anna) for now enqueueing to vol queue, need to
    // add system queue
    err = dm_req_handler->enqueueMsg(volume_id, snap_req);
    if (!err.ok()) {
        LOGERROR << "Failed to enqueue snap volcat request " << err;
    }
    return err;
}

void CatalogSync::snapDoneCb(const Error& error) {
    LOGDEBUG << "Will start rsync";
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

     meta_handler.reset(new FDSP_MetaSyncRpc(*this, GetLog()));

    std::string ip = netSession::getLocalIp();
    int port = PlatformProcess::plf_manager()->plf_get_my_metasync_port();
    int myIpInt = netSession::ipString2Addr(ip);
    std::string node_name = "localhost-meta";
    meta_session = netSessionTbl->createServerSession<netMetaSyncServerSession>(
        myIpInt,
        port,
        node_name,
        FDSP_METASYNC_MGR,
        meta_handler);

    LOGNORMAL << "Meta sync path server setup ip: "
              << ip << " port: " << port;

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
CatalogSyncMgr::startCatalogSync(const FDS_ProtocolInterface::FDSP_metaDataList& metaVolList) {
    Error err(ERR_OK);

    fds_mutex::scoped_lock l(cat_sync_lock);
    // For now we are not allowing to start new sync when current sync
    // is still in progress; note that we should remove sync jobs from
    // cat_sync_map when we are done with syncing
    if (cat_sync_map.size() > 0) {
        return ERR_NOT_READY;
    }

    for (auto metavol : metaVolList) {
      NodeUuid uuid(metavol.node_uuid.uuid);
      for (auto vol : metavol.volList) {
	LOGDEBUG << "Will sync vol " << std::hex << vol
		 << " to node " << uuid.uuid_get_val() << std::dec;
	fds_verify(cat_sync_map.count(vol) == 0);
	CatalogSyncPtr catsync(new CatalogSync(vol, uuid, dm_req_handler)); 
	cat_sync_map[vol] = catsync;

        // TODO(xxx) only start max_syn_inprogress syncs, but for now
        // starting all
	catsync->startSync(std::bind(
            &CatalogSyncMgr::syncDoneCb, this,
            std::placeholders::_1, std::placeholders::_2));
      }
    }

    return err;
}

void CatalogSyncMgr::syncDoneCb(fds_volid_t volid, const Error& error) {
    LOGNORMAL << "Sync is finished for volume " << std::hex << volid
              << std::dec << " " << error;
}

}  // namespace fds

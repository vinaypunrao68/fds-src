/*                                                                                                                                          
 * Copyright 2014 Formation Data Systems, Inc.                                                                                              
 */

#include <util/Log.h>
#include <fds_assert.h>
#include <fds_typedefs.h>
#include <lib/OMgrClient.h>
#include <CatalogSync.h>
#include "DataMgr.h"

namespace fds {

  extern DataMgr *dataMgr;
/****** CatalogSync implementation ******/

CatalogSync::CatalogSync(fds_volid_t vol_id,
                         const NodeUuid& uuid,
                         DmIoReqHandler* dm_req_hdlr)
        : volume_id(vol_id),
          node_uuid(uuid),
          state(CSSTATE_READY),
          dm_req_handler(dm_req_hdlr) {
      // create snap directory.
  const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
  const std::string snap_dir = root->dir_user_repo_snap();

   std::system((const char *)("mkdir "+snap_dir+" ").c_str());
}

CatalogSync::~CatalogSync() {
}

Error CatalogSync::startSync(catsync_done_handler_t done_evt_hdlr) {
    Error err(ERR_OK);
    fds_verify(state == CSSTATE_READY);
    LOGNORMAL << "Start sync for volume " << volume_id;

    // set callback to notify when sync job is done
    done_evt_handler = done_evt_hdlr;

    // queue qos request to do db snapshot and do rsync
    DmIoSnapVolCat* snap_req = new DmIoSnapVolCat();
    snap_req->io_type = FDS_DM_SNAP_VOLCAT;
    snap_req->node_uuid = node_uuid;
    snap_req->volId = volume_id;
    snap_req->dmio_snap_vcat_cb = std::bind(
        &CatalogSync::snapDoneCb, this,
        std::placeholders::_1, std::placeholders::_2);

    // enqueue to qos queue
    // TODO(anna) for now enqueueing to vol queue, need to
    // add system queue
    err = dm_req_handler->enqueueMsg(volume_id, snap_req);
    if (err.ok()) {
        state = CSSTATE_IN_PROGRESS;
    } else {
        LOGERROR << "Failed to enqueue snap volcat request " << err;
    }
    return err;
}

void CatalogSync::snapDoneCb(const Error& error, OMgrClient* omclient) {
    LOGDEBUG << "Will notify CatSyncMgr we finished";
    // at this moment we are finishing the whole volume
    // sync here, so notifying cat sync mgr we finished
    fds_verify(state == CSSTATE_IN_PROGRESS);
    state = CSSTATE_DONE;
    done_evt_handler(volume_id, omclient, error);
}

/***** CatalogSyncManager implementation ******/

CatalogSyncMgr::CatalogSyncMgr(fds_uint32_t max_jobs,
                               DmIoReqHandler* dm_req_hdlr,
                               netSessionTblPtr netSession)
        : Module("CatalogSyncMgr"),
          sync_in_progress(false),
          max_sync_inprogress(max_jobs),
          dm_req_handler(dm_req_hdlr),
          netSessionTbl(netSession),          
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

   meta_client  = NULL;
   meta_session->listenServerNb(); 
}


 // create client  session
netMetaSyncClientSession*
CatalogSyncMgr::get_metaSync_client(const std::string &ip, const int &port)
{
    /* Create a client rpc session from src to dst */
    netMetaSyncClientSession* meta_client_session =
            netSessionTbl->startSession<netMetaSyncClientSession>(
                    ip,
                    port,
                    FDSP_METASYNC_MGR,
                    1, /* number of channels */
                    meta_handler);

    LOGNORMAL << "Meta sync path Client setup ip: "
              << ip << " port: " << port <<
              "client_session:" << meta_client_session;
    return meta_client_session;
}


/**
 * Module shutdown code
 */
void CatalogSyncMgr::mod_shutdown()
{
    LOGNORMAL << "Ending metadata path sessions";
    netSessionTbl->endSession(meta_session->getSessionTblKey());
}

/**
 * Called when DM receives push meta message from OM to start catalog sync process for list
 * of vols and to which nodes to sync.
 */
Error
CatalogSyncMgr::startCatalogSync(const FDS_ProtocolInterface::FDSP_metaDataList& metaVolList,
                                 const std::string& context) {
    Error err(ERR_OK);
    fds_uint32_t node_ip   = 0;
    fds_uint32_t node_port = 0;
    fds_int32_t node_state = -1;
    fds_uint32_t sync_port = 0;
    

    fds_mutex::scoped_lock l(cat_sync_lock);
    // we should not get request to start a sync until
    // close is finished -- OM serializes DM deployment, so return an error
    if (sync_in_progress) {
        return ERR_NOT_READY;
    }
    fds_verify(cat_sync_map.size() == 0);

    // remember context to return with callback
    sync_in_progress = true;
    cat_sync_context = context;

    for (auto metavol : metaVolList) {
      NodeUuid uuid(metavol.node_uuid.uuid);
      // for each destination node setup the client  and init the session
      // get the IP and port of destination 
      dataMgr->omClient->getNodeInfo(uuid.uuid_get_val(), &node_ip, &node_port, &node_state);
      std::string dest_ip = netSessionTbl::ipAddr2String(node_ip);
      sync_port = dataMgr->omClient->getNodeMetaSyncPort(uuid);

       LOGDEBUG << " Push Meta dest IP:" << dest_ip  << " port : " << sync_port;
      // get the client session
       meta_client  = get_metaSync_client(dest_ip, sync_port);

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
            std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));
      }
    }

    return err;
}

void CatalogSyncMgr::syncDoneCb(fds_volid_t volid,
                                OMgrClient* omclient,
                                const Error& error) {
    fds_bool_t send_meta_push_ack = false;
    {  // check if all cat sync jobs are finished
        fds_mutex::scoped_lock l(cat_sync_lock);
        if (sync_in_progress) {
            send_meta_push_ack = true;
            for (CatSyncMap::const_iterator cit = cat_sync_map.cbegin();
                 cit != cat_sync_map.cend();
                 ++cit) {
                if ((cit->second)->isDone() == false) {
                    send_meta_push_ack = false;
                    break;
                }
            }
            sync_in_progress = (send_meta_push_ack == false);
        }
    }

    LOGNORMAL << "Sync is finished for volume " << std::hex << volid
              << std::dec << " " << error;

    if (send_meta_push_ack) {
        LOGNORMAL << "Catalog Sync finished for all volumes";
        omclient->sendDMTPushMetaAck(error, cat_sync_context);
    }
}

/**
 * Called when OM sends DMT close message to DM, so we can cleanup the
 * state of this sync and be ready for next sync...
 */
void CatalogSyncMgr::notifyCatalogSyncFinish() {
    // must be called when catalog sync is finished for all vols!
    fds_mutex::scoped_lock l(cat_sync_lock);
    fds_verify(sync_in_progress == false);

    // cleanup
    cat_sync_map.clear();

    // TODO(xxx) maybe close client connections to other DMs
    // we pushed meta?

    LOGDEBUG << "Cleaned up cat sync state";
}

  // meta sync done message to  new Node 

void CatalogSyncMgr::SendMetaSyncDone(fds_volid_t volid, NodeUuid dst_node_uuid) {
 

    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_VolMetaStatePtr vol_meta(new FDSP_VolMetaState);

    
    dataMgr->InitMsgHdr(msg_hdr);  // init the  message  header
    msg_hdr->dst_id = FDSP_DATA_MGR;


    LOGNORMAL << "Sending  MetaSyncDone Rpc message : " << meta_client;
    meta_client->getClient()->MetaSyncDone(msg_hdr, vol_meta);
}

void
FDSP_MetaSyncRpc::MetaSyncDone(FDSP_MsgHdrTypePtr& fdsp_msg,
                               FDSP_VolMetaStatePtr& vol_meta) {

    LOGNORMAL << "Received MetaSyncDone Rpc message ";
}


}  // namespace fds

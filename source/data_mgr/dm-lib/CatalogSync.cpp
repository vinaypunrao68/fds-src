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

/****** CatalogSync implementation ******/

CatalogSync::CatalogSync(const NodeUuid& uuid,
                         OMgrClient* omclient,
                         DmIoReqHandler* dm_req_hdlr)
        : node_uuid(uuid),
          state(CSSTATE_READY),
          om_client(omclient),
          meta_client(NULL),
          dm_req_handler(dm_req_hdlr) {
    state = ATOMIC_VAR_INIT(CSSTATE_READY);
    vols_done = ATOMIC_VAR_INIT(0);

    // create snap directory.
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    const std::string snap_dir = root->dir_user_repo_snap();

    std::system((const char *)("mkdir "+snap_dir+" ").c_str());
}

CatalogSync::~CatalogSync() {
}

Error CatalogSync::startSync(const std::set<fds_volid_t>& volumes,
                             netMetaSyncClientSession* client,
                             catsync_done_handler_t done_evt_hdlr) {
    Error err(ERR_OK);
    csStateType expect = CSSTATE_READY;
    if (!std::atomic_compare_exchange_strong(&state, &expect, CSSTATE_IN_PROGRESS)) {
        LOGNOTIFY << "startSync called in not ready state, must not happen!";
        fds_verify(false);
        return ERR_NOT_READY;
    }

    LOGNORMAL << "Start sync to destination node "
              << std::hex << node_uuid.uuid_get_val() << std::dec;

    // set callback to notify when sync job is done
    meta_client = client;
    done_evt_handler = done_evt_hdlr;
    total_vols = 0;
    fds_uint32_t counter_zero = 0;
    std::atomic_store(&vols_done, counter_zero);

    // queue qos requests to do db snapshot and do rsync per volume!
    for (std::set<fds_volid_t>::const_iterator cit = volumes.cbegin();
         cit != volumes.cend();
         ++cit) {
        DmIoSnapVolCat* snap_req = new DmIoSnapVolCat();
        snap_req->io_type = FDS_DM_SNAP_VOLCAT;
        snap_req->node_uuid = node_uuid;
        snap_req->volId = *cit;
        snap_req->dmio_snap_vcat_cb = std::bind(
            &CatalogSync::snapDoneCb, this,
            std::placeholders::_1, std::placeholders::_2);

        // enqueue to qos queue
        // TODO(anna) for now enqueueing to vol queue, need to
        // add system queue
        err = dm_req_handler->enqueueMsg(*cit, snap_req);
        // TODO(xxx) handle error -- probably queue is full and we
        // should retry later?
        fds_verify(err.ok());
    }

    return err;
}

void CatalogSync::snapDoneCb(fds_volid_t volid,
                             const Error& error) {
    fds_uint32_t done_before, total_done;
    fds_uint32_t vols_done_now = 1;  // doing 1 vol at a time

    // we must be in progress state
    csStateType cur_state = std::atomic_load(&state);
    fds_verify(cur_state == CSSTATE_IN_PROGRESS);

    if (!error.ok()) {
        LOGERROR << "Rsync finished with error!";
        // TODO(xxx) should we remember the error and continue with
        // other rsync???
        // for now ignoring...
    }

    // account for progress and see if we finished rsyncing all
    // volumes to this node
    done_before = std::atomic_fetch_add(&vols_done, vols_done_now);
    total_done = done_before + vols_done_now;
    fds_verify(total_done <= total_vols);

    LOGDEBUG << "Finished rsync for volume " << std::hex << volid
             << " to node " << node_uuid.uuid_get_val() << std::dec
             << ", " << error << "; volumes finished so far "
             << total_done << " out of " << total_vols;

    // Send msg to DM that we finished syncing this volume
    Error err = sendMetaSyncDone(volid);
    if (!err.ok()) {
        // TODO(xxx) how should we handle this? for now ignoring
    }

    if (total_done == total_vols) {
        // we are done for this node!
        std::atomic_exchange(&state, CSSTATE_DONE);
        // notify catsync mgr that we are done
        fds_verify(done_evt_handler);
        done_evt_handler(volid, om_client, error);
    }
}

fds_bool_t CatalogSync::isDone() const {
    csStateType cur_state = std::atomic_load(&state);
    return (cur_state == CSSTATE_DONE);
}

Error CatalogSync::sendMetaSyncDone(fds_volid_t volid) {
    Error err(ERR_OK);
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_VolMetaStatePtr vol_meta(new FDSP_VolMetaState);

    DataMgr::InitMsgHdr(msg_hdr);  // init the  message  header
    msg_hdr->dst_id = FDSP_DATA_MGR;

    try {
        meta_client->getClient()->MetaSyncDone(msg_hdr, vol_meta);
        LOGNORMAL << "Senr MetaSyncDone Rpc message : " << meta_client;
    } catch (...) {
        LOGERROR << "Unable to send MetaSyncDone to DM";
        err = ERR_NETWORK_TRANSPORT;
    }

    return err;
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

    meta_session->listenServerNb(); 
}


 // create client  session
netMetaSyncClientSession*
CatalogSyncMgr::create_metaSync_client(const NodeUuid& node_uuid, OMgrClient* omclient)
{
    fds_uint32_t node_ip   = 0;
    fds_uint32_t node_port = 0;
    fds_int32_t node_state = -1;
    fds_uint32_t sync_port = 0;

    omclient->getNodeInfo(node_uuid.uuid_get_val(), &node_ip, &node_port, &node_state);
    std::string dest_ip = netSessionTbl::ipAddr2String(node_ip);
    sync_port = omclient->getNodeMetaSyncPort(node_uuid);
    LOGDEBUG << "Dest node " << std::hex << node_uuid.uuid_get_val() << std::dec
             << "  = dest IP:" << dest_ip  << " port : " << sync_port;

    /* Create a client rpc session from src to dst */
    netMetaSyncClientSession* meta_client_session =
            netSessionTbl->startSession<netMetaSyncClientSession>(
                    dest_ip,
                    sync_port,
                    FDSP_METASYNC_MGR,
                    1, /* number of channels */
                    meta_handler);

    LOGNORMAL << "Meta sync path Client setup ip: "
              << dest_ip << " port: " << sync_port <<
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
                                 OMgrClient* omclient,
                                 const std::string& context) {
    Error err(ERR_OK);
    
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

        // create CatalogSync object to handle syncing vols to node 'uuid'
        fds_verify(cat_sync_map.count(uuid) == 0);
        CatalogSyncPtr catsync(new CatalogSync(uuid, omclient, dm_req_handler));
        cat_sync_map[uuid] = catsync;

        // Get a set of volume ids that this node will need to push to node 'uuid'
        std::set<fds_volid_t> vols;

        for (auto vol : metavol.volList) {
            LOGDEBUG << "Will sync vol " << std::hex << vol
                     << " to node " << uuid.uuid_get_val() << std::dec;
            vols.insert(vol);
        }

        // create client talking to node 'uuid' and pass it to CatalogSync
        netMetaSyncClientSession* meta_client = create_metaSync_client(uuid, omclient);

        // Tell CatalogSync to start the sync process
        // TODO(xxx) only start max_syn_inprogress syncs, but for now starting all
        catsync->startSync(vols, meta_client,
                           std::bind(&CatalogSyncMgr::syncDoneCb, this,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3));
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

void
FDSP_MetaSyncRpc::MetaSyncDone(FDSP_MsgHdrTypePtr& fdsp_msg,
                               FDSP_VolMetaStatePtr& vol_meta) {

    LOGNORMAL << "Received MetaSyncDone Rpc message ";
}


}  // namespace fds

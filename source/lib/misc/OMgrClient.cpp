/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include "./OMgrClient.h"
#include <fds_assert.h>
#include <boost/thread.hpp>
#include <NetSession.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TBufferTransports.h>
#include <dlt.h>

#include <net/net_utils.h>
#include <net/SvcRequestPool.h>
#include "platform/platform_process.h"
#include "platform/platform.h"
#include <fds_typedefs.h>
#include "fdsp/om_service_types.h"
#include <thread>
#include <string>
using namespace std; // NOLINT
using namespace fds; // NOLINT

namespace fds {
extern const NodeUuid gl_OmUuid;
extern SvcRequestPool *gSvcRequestPool;

OMgrClientRPCI::OMgrClientRPCI(OMgrClient *omc) {
    this->om_client = omc;
}

OMgrClient::OMgrClient(FDSP_MgrIdType node_type,
                       const std::string& _omIpStr,
                       fds_uint32_t _omPort,
                       const std::string& node_name,
                       fds_log *parent_log,
                       boost::shared_ptr<netSessionTbl> nst,
                       Platform *plf,
                       fds_uint32_t _instanceId)
        : dltMgr(new DLTManager()),
          dmtMgr(new DMTManager(1)),
          instanceId(_instanceId) {
    fds_verify(_omPort != 0);
    my_node_type = node_type;
    omIpStr      = _omIpStr;
    omConfigPort = _omPort;
    my_node_name = node_name;
    node_evt_hdlr = NULL;
    bucket_stats_cmd_hdlr = NULL;
    dltclose_evt_hdlr = NULL;
    if (parent_log) {
        omc_log = parent_log;
    } else {
        omc_log = new fds_log("omc", "logs");
    }

    // TODO(Andrew): Need to completely remove the NetSession references. Right
    // now we're stuck with it because RegisterNode hasn't been moved to service
    // layer. Maybe just removing this whole file is easier (hint, hint).
    if (nullptr == nst_) {
        // Make up a netsession server to use if the user of the library has
        // freed itself of netsession.
        nst_ = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_MGR));
    } else {
        nst_ = nst;
    }

    clustMap = new LocalClusterMap();
    plf_mgr  = plf;
    fNoNetwork = false;
}

OMgrClient::~OMgrClient()
{
    nst_->endSession(omrpc_handler_session_->getSessionTblKey());
    omrpc_handler_thread_->join();

    delete clustMap;
}

int OMgrClient::initialize() {
    return fds::ERR_OK;
}

int OMgrClient::registerEventHandlerForMigrateEvents(migration_event_handler_t migrate_event_hdlr) {
    this->migrate_evt_hdlr = migrate_event_hdlr;
    return 0;
}

int OMgrClient::registerEventHandlerForDltCloseEvents(dltclose_event_handler_t dltclose_event_hdlr) { //NOLINT
    this->dltclose_evt_hdlr = dltclose_event_hdlr;
    return 0;
}

int OMgrClient::registerBucketStatsCmdHandler(bucket_stats_cmd_handler_t cmd_hdlr) {
    bucket_stats_cmd_hdlr = cmd_hdlr;
    return 0;
}

/**
 * @brief Starts OM RPC handling server.  This function is to be run on a
 * separate thread.  OMgrClient destructor does a join() on this thread
 */
void OMgrClient::start_omrpc_handler()
{
    if (fNoNetwork) return;
    try {
        nst_->listenServer(omrpc_handler_session_);
    } catch(const att::TTransportException& e) {
        LOGERROR << "unable to listen at the given port - check the port";
        LOGERROR << "error during network call : " << e.what();
        fds_panic("Unable to listen on server...");
    }
}

// Call this to setup the (receiving side) endpoint to lister for control path requests from OM.
int OMgrClient::startAcceptingControlMessages() {
    if (fNoNetwork) return 0;
    std::string myIp = fds::net::get_local_ip(
        g_fdsprocess->get_fds_config()->get<std::string>("fds.nic_if"));
    int myIpInt = netSession::ipString2Addr(myIp);
    omrpc_handler_.reset(new OMgrClientRPCI(this));
    // TODO(x): Ideally createServerSession should take a shared pointer
    // for omrpc_handler_.  Make sure that happens.  Otherwise you
    // end up with a pointer leak.
    fds_uint32_t ctrlPort = plf_mgr->plf_get_my_ctrl_port() + instanceId;
    omrpc_handler_session_ =
            nst_->createServerSession<netControlPathServerSession>(myIpInt,
                                                                   ctrlPort,
                                                                   my_node_name,
                                                                   FDSP_ORCH_MGR,
                                                                   omrpc_handler_);
    omrpc_handler_thread_.reset(new boost::thread(&OMgrClient::start_omrpc_handler, this));

    LOGNOTIFY << "OMClient accepting control requests at port "
              << ctrlPort;

    return (0);
}

void OMgrClient::initOMMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
{
    msg_hdr->minor_ver = 0;
    msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_REQ;
    msg_hdr->msg_id =  1;

    msg_hdr->major_ver = 0xa5;
    msg_hdr->minor_ver = 0x5a;

    msg_hdr->num_objects = 1;
    msg_hdr->frag_len = 0;
    msg_hdr->frag_num = 0;

    msg_hdr->tennant_id = 0;
    msg_hdr->local_domain_id = 0;

    msg_hdr->src_id = my_node_type;
    msg_hdr->dst_id = FDSP_ORCH_MGR;
    msg_hdr->src_node_name = my_node_name;
    msg_hdr->src_service_uuid.uuid = myUuid.uuid_get_val();

    msg_hdr->err_code = ERR_OK;
    msg_hdr->result = FDSP_ERR_OK;
}

// Use this to register the local node with OM as a client.
// Should be called after calling starting subscription endpoint and control path endpoint.
int OMgrClient::registerNodeWithOM(Platform *plat)
{
    if (fNoNetwork) return 0;
    try {
        omclient_prx_session_ = nst_->startSession<netOMControlPathClientSession>(
            omIpStr, omConfigPort, FDSP_ORCH_MGR, 1, /* number of channels */
            boost::shared_ptr<FDSP_OMControlPathRespIf>());
        /* TODO:  pass in response path server pointer */
        // Just return if the om ptr is NULL because
        // FDS-net doesn't throw the exception we're
        // trying to catch. We should probably return
        // an error and let the caller decide what to do.
        // fds_verify(omclient_prx_session_ != nullptr);
        if (omclient_prx_session_ == NULL) {
            LOGCRITICAL << "OMClient unable to register node with OrchMgr. "
                    "Please check if OrchMgr is up and restart.";
            return 0;
        }
        om_client_prx = omclient_prx_session_->getClient();  // NOLINT

        FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
        initOMMsgHdr(msg_hdr);

        FDSP_RegisterNodeTypePtr reg_node_msg(new FDSP_RegisterNodeType);
        reg_node_msg->node_type    = plat->plf_get_node_type();
        reg_node_msg->node_name    = *plat->plf_get_my_name();
        reg_node_msg->ip_hi_addr   = 0;
        reg_node_msg->ip_lo_addr   = fds::str_to_ipv4_addr(*plat->plf_get_my_ip());
        reg_node_msg->control_port = plat->plf_get_my_ctrl_port();
        reg_node_msg->data_port    = plat->plf_get_my_data_port();
        reg_node_msg->node_root    = g_fdsprocess->proc_fdsroot()->dir_fdsroot();

        // TODO(Andrew): Move to SM specific
        reg_node_msg->migration_port = plat->plf_get_my_migration_port();
        reg_node_msg->metasync_port  = plat->plf_get_my_metasync_port();

        // TODO(Vy): simple service uuid from node uuid.
        reg_node_msg->node_uuid.uuid    = plat->plf_get_my_node_uuid()->uuid_get_val();
        reg_node_msg->service_uuid.uuid = plat->plf_get_my_svc_uuid()->uuid_get_val();
        myUuid.uuid_set_val(plat->plf_get_my_svc_uuid()->uuid_get_val());

        LOGNOTIFY << "OMClient registering local node "
                  << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr)
                  << " control port:" << reg_node_msg->control_port
                  << " data port:" << reg_node_msg->data_port
                  << " with Orchaestration Manager at "
                  << omIpStr << ":" << omConfigPort;

        om_client_prx->RegisterNode(msg_hdr, reg_node_msg);
        LOGDEBUG << "OMClient completed node registration with OM";
    }
    catch(...) {
        LOGCRITICAL << "OMClient unable to register node with OrchMgr. "
                "Please check if OrchMgr is up and restart.";
    }
    return (0);
}

int OMgrClient::testBucket(const std::string& bucket_name,
                           const fpi::FDSP_VolumeDescTypePtr& vol_info,
                           fds_bool_t attach_vol_reqd,
                           const std::string& accessKeyId,
                           const std::string& secretAccessKey)
{
    if (fNoNetwork) return 0;
    try {
        auto req =  gSvcRequestPool->newEPSvcRequest(gl_OmUuid.toSvcUuid());
        fpi::CtrlTestBucketPtr pkt(new fpi::CtrlTestBucket());
        fpi::FDSP_TestBucket * test_buck_msg = & pkt->tbmsg;
        test_buck_msg->bucket_name = bucket_name;
        test_buck_msg->vol_info = *vol_info;
        test_buck_msg->attach_vol_reqd = attach_vol_reqd;
        test_buck_msg->accessKeyId = accessKeyId;
        test_buck_msg->secretAccessKey;
        req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlTestBucket), pkt);
        req->invoke();
        LOGNOTIFY << " sending test bucket request to OM " << bucket_name;
    } catch(...) {
        LOGERROR << "OMClient unable to push test bucket to OM. Check if OM is up and restart.";
    }
    return 0;
}

int OMgrClient::recvMigrationEvent(bool dlt_type)
{
    LOGNOTIFY << "OMClient received Migration event for node " << dlt_type;

    if (this->migrate_evt_hdlr) {
        this->migrate_evt_hdlr(dlt_type);
    }
    return (0);
}

Error OMgrClient::updateDlt(bool dlt_type, std::string& dlt_data) {
    Error err(ERR_OK);
    LOGNOTIFY << "OMClient received new DLT version  " << dlt_type;

    // dltMgr is threadsafe
    err = dltMgr->addSerializedDLT(dlt_data, NULL, dlt_type);
    if (err.ok()) {
        dltMgr->dump();
    } else {
        LOGERROR << "Failed to update DLT! check dlt_data was set " << err;
    }

    return err;
}

Error OMgrClient::updateDmt(bool dmt_type, std::string& dmt_data) {
    Error err(ERR_OK);
    LOGNOTIFY << "OMClient received new DMT version  " << dmt_type;

    omc_lock.write_lock();
    err = dmtMgr->addSerializedDMT(dmt_data, DMT_COMMITTED);
    if (!err.ok()) {
        LOGERROR << "Failed to update DMT! check dmt_data was set";
    }
    omc_lock.write_unlock();

    return err;
}

int
OMgrClient::getNodeInfo(fds_uint64_t node_id,
                        unsigned int *node_ip_addr,
                        fds_uint32_t *node_port,
                        int *node_state) {
    return clustMap->getNodeInfo(node_id,
                                 node_ip_addr,
                                 node_port,
                                 node_state);
}

fds_uint32_t OMgrClient::getLatestDlt(std::string& dlt_data) {
    // TODO(x): Set to a macro'd invalid version
    // dltMgr is threadsafe, no need to take a lock
    Error err = const_cast <DLT* > (dltMgr->getDLT())->getSerialized(dlt_data);
    fds_verify(err.ok());
    return 0;
}

const DLT* OMgrClient::getCurrentDLT() {
    return dltMgr->getDLT();
}

void OMgrClient::setCurrentDLTClosed() {
    dltMgr->setCurrentDltClosed();
}

const DLT*
OMgrClient::getPreviousDLT() {
    fds_uint64_t version = (dltMgr->getDLT()->getVersion()) - 1;
    const DLT *dlt = dltMgr->getDLT(version);
    return dlt;
}

fds_uint64_t
OMgrClient::getDltVersion() {
    return dltMgr->getDLT()->getVersion();
}

NodeUuid
OMgrClient::getUuid() const {
    return myUuid;
}

FDSP_MgrIdType
OMgrClient::getNodeType() const {
    return my_node_type;
}

const TokenList&
OMgrClient::getTokensForNode(const NodeUuid &uuid) const {
    return dltMgr->getDLT()->getTokens(uuid);
}

fds_uint32_t
OMgrClient::getNodeMigPort(NodeUuid uuid) {
    return clustMap->getNodeMigPort(uuid);
}

fds_uint32_t
OMgrClient::getNodeMetaSyncPort(NodeUuid uuid) {
    return clustMap->getNodeMetaSyncPort(uuid);
}


NodeMigReqClientPtr
OMgrClient::getMigClient(fds_uint64_t node_id) {
    return clustMap->getMigClient(node_id);
}

DltTokenGroupPtr OMgrClient::getDLTNodesForDoidKey(const ObjectID &objId) {
    return dltMgr->getDLT()->getNodes(objId);
}

DmtColumnPtr OMgrClient::getDMTNodesForVolume(fds_volid_t vol_id) {
    return dmtMgr->getCommittedNodeGroup(vol_id);  // thread-safe, do not hold lock
}

DmtColumnPtr OMgrClient::getDMTNodesForVolume(fds_volid_t vol_id,
                                              fds_uint64_t dmt_version) {
    return dmtMgr->getVersionNodeGroup(vol_id, dmt_version);  // thread-safe, do not hold lock
}

fds_uint64_t OMgrClient::getDMTVersion() const {
    return dmtMgr->getCommittedVersion();
}

fds_bool_t OMgrClient::hasCommittedDMT() const {
    return dmtMgr->hasCommittedDMT();
}

}  //  namespace fds

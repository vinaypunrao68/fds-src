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
#include <net/SvcMgr.h>
#include "platform/platform_process.h"
#include "platform/platform.h"
#include <fds_typedefs.h>
#include "fdsp/om_api_types.h"
#include <thread>
#include <string>
using namespace std; // NOLINT
using namespace fds; // NOLINT

namespace fds {
extern SvcRequestPool *gSvcRequestPool;

OMgrClient::OMgrClient(FDSP_MgrIdType node_type,
                       const std::string& _omIpStr,
                       fds_uint32_t _omPort,
                       const std::string& node_name,
                       fds_log *parent_log,
                       boost::shared_ptr<netSessionTbl> nst,
                       Platform *plf)
        : dltMgr(new DLTManager()),
          dmtMgr(new DMTManager(1)) {
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

    fNoNetwork = false;
}

OMgrClient::~OMgrClient()
{
    delete clustMap;
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

int OMgrClient::testBucket(const std::string& bucket_name,
                           fds_bool_t attach_vol_reqd,
                           const std::string& accessKeyId,
                           const std::string& secretAccessKey)
{
    if (fNoNetwork) return 0;
    try {
        auto req =  gSvcRequestPool->newEPSvcRequest(MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());
        fpi::CtrlTestBucketPtr pkt(new fpi::CtrlTestBucket());
        fpi::FDSP_TestBucket * test_buck_msg = & pkt->tbmsg;
        test_buck_msg->bucket_name = bucket_name;
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

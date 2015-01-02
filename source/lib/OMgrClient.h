/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef  SOURCE_LIB_OMGRCLIENT_H_
#define  SOURCE_LIB_OMGRCLIENT_H_
#include <fds_error.h>
#include <fds_volume.h>

#include "fdsp/FDSP_types.h"
#include "fdsp/FDSP_ControlPathReq.h"
#include "fdsp/FDSP_OMControlPathReq.h"
#include <util/Log.h>

#include <unordered_map>
#include <concurrency/RwLock.h>
#include <net-proxies/vol_policy.h>
#include <dlt.h>
#include <fds_dmt.h>
#include <LocalClusterMap.h>

#include <string>
using namespace FDS_ProtocolInterface; // NOLINT

#define FDS_VOL_ACTION_NONE   0
#define FDS_VOL_ACTION_CREATE 1
#define FDS_VOL_ACTION_DELETE 2
#define FDS_VOL_ACTION_MODIFY 3
#define FDS_VOL_ACTION_ATTACH 4
#define FDS_VOL_ACTION_DETACH 5

namespace FDS_ProtocolInterface {
class FDSP_OMControlPathReqClient;
class FDSP_OMControlPathRespProcessor;
class FDSP_OMControlPathRespIf;
class FDSP_ControlPathRespClient;
}  // namespace FDS_ProtocolInterface

template <class A, class B, class C> class netClientSessionEx;
template <class A, class B, class C> class netServerSessionEx;
typedef netClientSessionEx<FDSP_OMControlPathReqClient,
                           FDSP_OMControlPathRespProcessor,
                           FDSP_OMControlPathRespIf> netOMControlPathClientSession;
typedef netServerSessionEx<FDSP_ControlPathReqProcessor,
                           FDSP_ControlPathReqIf,
                           FDSP_ControlPathRespClient> netControlPathServerSession;

namespace fds {

class Platform;

typedef enum {
    fds_notify_vol_default = 0,
    fds_notify_vol_add     = 1,
    fds_notify_vol_rm      = 2,
    fds_notify_vol_mod     = 3,
    fds_notify_vol_attatch = 4,
    fds_notify_vol_detach  = 5,
    fds_notify_vol_snap    = 6,
    MAX
} fds_vol_notify_t;

typedef enum {
    fds_catalog_push_meta = 0,
    fds_catalog_dmt_commit = 1,
    fds_catalog_dmt_close = 2
} fds_catalog_action_t;

typedef void (*migration_event_handler_t)(bool dlt_type);
typedef void (*dltclose_event_handler_t)(FDSP_DltCloseTypePtr& dlt_close,
                                         const std::string& session_uuid);
typedef void (*node_event_handler_t)(int node_id,
                                     unsigned int node_ip_addr,
                                     int node_state,
                                     fds_uint32_t node_port,
                                     FDSP_MgrIdType node_type);
typedef void (*tier_cmd_handler_t)(const FDSP_TierPolicyPtr &tier);
typedef void (*tier_audit_cmd_handler_t)(const FDSP_TierPolicyAuditPtr &tier);
typedef void (*bucket_stats_cmd_handler_t)(const FDSP_MsgHdrTypePtr& rx_msg,
                                           const FDSP_BucketStatsRespTypePtr& buck_stats);
typedef Error (*catalog_event_handler_t)(fds_catalog_action_t cat_action,
                                         const FDSP_PushMetaPtr& push_meta,
                                         const std::string& session_uuid);

class OMgrClient {
  protected:
    int tennant_id;
    int domain_id;
    FDSP_MgrIdType my_node_type;
    NodeUuid myUuid;
    bool fNoNetwork;
    std::string my_node_name;
    std::string omIpStr;
    fds_uint32_t omConfigPort;
    const DLT *dlt;
    DLTManagerPtr dltMgr;
    DMTManagerPtr dmtMgr;
    float current_throttle_level;

    /**
     * Map of current cluster members
     */
    LocalClusterMap *clustMap;
    Platform        *plf_mgr;

    fds_rwlock omc_lock;  // to protect node_map

    node_event_handler_t node_evt_hdlr;
    migration_event_handler_t migrate_evt_hdlr;
    dltclose_event_handler_t dltclose_evt_hdlr;
    tier_cmd_handler_t       tier_cmd_hdlr;
    tier_audit_cmd_handler_t tier_audit_cmd_hdlr;
    bucket_stats_cmd_handler_t bucket_stats_cmd_hdlr;
    catalog_event_handler_t catalog_evt_hdlr;

    /**
     * Session table for OM client
     */
    boost::shared_ptr<netSessionTbl> nst_;

    /**
     * RPC handler for request coming from OM
     */
    boost::shared_ptr<FDS_ProtocolInterface::FDSP_ControlPathReqIf> omrpc_handler_;
    /**
     * Session associated with omrpc_handler_
     */
    netControlPathServerSession *omrpc_handler_session_;
    /**
     * omrpc_handler_ server is run on this thread
     */
    boost::shared_ptr<boost::thread> omrpc_handler_thread_;

    /**
     * client for sending messages to OM
     */
    netOMControlPathClientSession* omclient_prx_session_;
    boost::shared_ptr<FDS_ProtocolInterface::FDSP_OMControlPathReqClient> om_client_prx;

    void initOMMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr);

    /// Tracks local instances (only used for multi-AM at the moment)
    fds_uint32_t instanceId;

  public:
    OMgrClient(fpi::FDSP_MgrIdType node_type,
               const std::string& _omIpStr,
               fds_uint32_t _omPort,
               const std::string& node_name,
               fds_log *parent_log,
               boost::shared_ptr<netSessionTbl> nst,
               Platform *plf_mgr,
               fds_uint32_t _instanceId = 0);
    void setNoNetwork(bool fNoNetwork) {
        this->fNoNetwork = fNoNetwork;
    }
    ~OMgrClient();
    int initialize();
    void start_omrpc_handler();

    NodeUuid getUuid() const;
    FDSP_MgrIdType getNodeType() const;

    int registerEventHandlerForNodeEvents(node_event_handler_t node_event_hdlr);
    int registerEventHandlerForMigrateEvents(migration_event_handler_t migrate_event_hdlr);
    int registerEventHandlerForDltCloseEvents(dltclose_event_handler_t dltclose_event_hdlr);
    int registerBucketStatsCmdHandler(bucket_stats_cmd_handler_t cmd_hdlr);
    void registerCatalogEventHandler(catalog_event_handler_t evt_hdlr);

    // This logging is public for external plugins.  Avoid making this object
    // too big and all methods uses its data as global variables with big lock.
    //
    fds_log        *omc_log;

    // Extneral plugin object to handle policy requests.
    VolPolicyServ  *omc_srv_pol;

    // int subscribeToOmEvents(unsigned int om_ip_addr,
    // int tennant_id, int domain_id, int omc_port_num= 0);
    int startAcceptingControlMessages();
    int registerNodeWithOM(Platform *plat);
    int pushCreateBucketToOM(const FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr& volInfo);
    int pushDeleteBucketToOM(const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr& volInfo);
    int pushModifyBucketToOM(const std::string& bucket_name,
                             const FDS_ProtocolInterface::FDSP_VolumeDescTypePtr& vol_desc);
    int pushGetBucketStatsToOM(fds_uint32_t req_cookie);
    int sendMigrationStatusToOM(const Error& err);

    int getNodeInfo(fds_uint64_t node_id,
                    unsigned int *node_ip_addr,
                    fds_uint32_t *node_port,
                    int *node_state);
    NodeMigReqClientPtr getMigClient(fds_uint64_t node_id);

    fds_uint64_t getDltVersion();
    fds_uint32_t getLatestDlt(std::string& dlt_data);
    DltTokenGroupPtr getDLTNodesForDoidKey(const ObjectID &objId);
    const DLT* getCurrentDLT();
    const DLT* getPreviousDLT();
    const TokenList& getTokensForNode(const NodeUuid &uuid) const;
    fds_uint32_t getNodeMigPort(NodeUuid uuid);
    fds_uint32_t getNodeMetaSyncPort(NodeUuid uuid);

    DLTManagerPtr getDltManager() { return dltMgr; }
    DMTManagerPtr getDmtManager() { return dmtMgr; }

    /**
     * Returns nodes from currently committed DMT
     */
    DmtColumnPtr getDMTNodesForVolume(fds_volid_t vol_id);
    /**
     * Returns nodes from specific DMT version 'dmt_version'
     */
    DmtColumnPtr getDMTNodesForVolume(fds_volid_t vol_id, fds_uint64_t dmt_version);
    fds_uint64_t getDMTVersion() const;
    int pushPerfstatsToOM(const std::string& start_ts,
                          int stat_slot_len,
                          const FDS_ProtocolInterface::FDSP_VolPerfHistListType& hist_list);
    int testBucket(const std::string& bucket_name,
                   const FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr& vol_info,
                   fds_bool_t attach_vol_reqd,
                   const std::string& accessKeyId,
                   const std::string& secretAccessKey);

    int recvNodeEvent(int node_id,
                      FDSP_MgrIdType node_type,
                      unsigned int node_ip,
                      int node_state,
                      const FDSP_Node_Info_TypePtr& node_info);
    int recvMigrationEvent(bool dlt_type);
    Error updateDlt(bool dlt_type, std::string& dlt_data);
    int recvDLTUpdate(FDSP_DLT_Data_TypePtr& dlt_info, const std::string& session_uuid);
    int recvDLTClose(FDSP_DltCloseTypePtr& close_info, const std::string& session_uuid);
    int sendDLTCloseAckToOM(FDSP_DltCloseTypePtr& dlt_close,
                            const std::string& session_uuid);
    Error recvDLTStartMigration(FDSP_DLT_Data_TypePtr& dlt_info);
    Error recvDMTUpdate(FDSP_DMT_TypePtr& dmt_info, const std::string& session_uuid);
    Error recvDMTPushMeta(FDSP_PushMetaPtr& push_meta, const std::string& session_uuid);
    Error sendDMTPushMetaAck(const Error& op_err, const std::string& session_uuid);
    Error sendDMTCommitAck(const Error& op_err, const std::string& session_uuid);

    void recvDMTClose(fds_uint64_t dmt_version, const std::string& session_uuid);
    Error updateDmt(bool dmt_type, std::string& dmt_data);
    int sendDMTCloseAckToOM(FDSP_DmtCloseTypePtr& dmt_close,
                            const std::string& session_uuid);

    int recvTierPolicy(const FDSP_TierPolicyPtr &tier);
    int recvTierPolicyAudit(const FDSP_TierPolicyAuditPtr &audit);
    int recvBucketStats(const FDSP_MsgHdrTypePtr& msg_hdr,
                        const FDSP_BucketStatsRespTypePtr& buck_stats_msg);
};

class OMgrClientRPCI : public FDS_ProtocolInterface::FDSP_ControlPathReqIf {
  private:
    OMgrClient *om_client;

  public:
    explicit OMgrClientRPCI(OMgrClient *om_c);

    void NotifyAddVol(const FDSP_MsgHdrType& fdsp_msg,
                      const FDSP_NotifyVolType& not_add_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyAddVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                      FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg);

    void NotifyRmVol(const FDSP_MsgHdrType& fdsp_msg,
                     const FDSP_NotifyVolType& not_rm_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyRmVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg);

    void NotifyModVol(const FDSP_MsgHdrType& fdsp_msg,
                      const FDSP_NotifyVolType& not_mod_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyModVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                      FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg);

    void NotifySnapVol(const FDSP_MsgHdrType& fdsp_msg,
                       const FDSP_NotifyVolType& not_snap_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifySnapVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                       FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg);


    void AttachVol(const FDSP_MsgHdrType& fdsp_msg, const FDSP_AttachVolType& atc_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void AttachVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                   FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg);


    void DetachVol(const FDSP_MsgHdrType& fdsp_msg,
                   const FDSP_AttachVolType& dtc_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void DetachVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                   FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg);

    void NotifyNodeAdd(const FDSP_MsgHdrType& fdsp_msg,
                       const FDSP_Node_Info_Type& node_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyNodeAdd(FDSP_MsgHdrTypePtr& msg_hdr,
                       FDSP_Node_Info_TypePtr& node_info);

    void NotifyNodeActive(const FDSP_MsgHdrType& fdsp_msg,
                          const FDSP_ActivateNodeType& act_node_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyNodeActive(FDSP_MsgHdrTypePtr& msg_hdr,
                          FDSP_ActivateNodeTypePtr& act_node_req) {}

    void NotifyNodeRmv(const FDSP_MsgHdrType& fdsp_msg,
                       const FDSP_Node_Info_Type& node_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyNodeRmv(FDSP_MsgHdrTypePtr& msg_hdr,
                       FDSP_Node_Info_TypePtr& node_info);

    void NotifyDLTUpdate(const FDSP_MsgHdrType& fdsp_msg,
                         const FDSP_DLT_Data_Type& dlt_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyDLTUpdate(FDSP_MsgHdrTypePtr& msg_hdr,
                         FDSP_DLT_Data_TypePtr& dlt_info);

    void NotifyDLTClose(const FDSP_MsgHdrType& fdsp_msg,
                        const FDSP_DltCloseType& dlt_close) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyDLTClose(FDSP_MsgHdrTypePtr& fdsp_msg,
                        FDSP_DltCloseTypePtr& dlt_close);

    void PushMetaDMTReq(const FDSP_MsgHdrType& fdsp_msg,
                        const FDSP_PushMeta& push_meta_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void PushMetaDMTReq(FDSP_MsgHdrTypePtr& fdsp_msg,
                        FDSP_PushMetaPtr& push_meta_req);

    void NotifyDMTClose(const FDSP_MsgHdrType& fdsp_msg,
                        const FDSP_DmtCloseType& dmt_close) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyDMTClose(FDSP_MsgHdrTypePtr& fdsp_msg,
                        FDSP_DmtCloseTypePtr& dmt_close);

    void NotifyDMTUpdate(const FDSP_MsgHdrType& msg_hdr,
                         const FDSP_DMT_Type& dmt_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyDMTUpdate(FDSP_MsgHdrTypePtr& msg_hdr,
                         FDSP_DMT_TypePtr& dmt_info);

    void TierPolicy(const FDSP_TierPolicy &tier) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void TierPolicy(FDSP_TierPolicyPtr &tier);

    void TierPolicyAudit(const FDSP_TierPolicyAudit &audit) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void TierPolicyAudit(FDSP_TierPolicyAuditPtr &audit);

    void NotifyBucketStats(const FDS_ProtocolInterface::FDSP_MsgHdrType& msg_hdr,
                           const FDS_ProtocolInterface::FDSP_BucketStatsRespType& buck_stats_msg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyBucketStats(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                           FDS_ProtocolInterface::FDSP_BucketStatsRespTypePtr& buck_stats_msg);


    void NotifyStartMigration(const FDS_ProtocolInterface::FDSP_MsgHdrType& msg_hdr,
                              const FDS_ProtocolInterface::FDSP_DLT_Data_Type& dlt_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyStartMigration(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                              FDS_ProtocolInterface::FDSP_DLT_Data_TypePtr& dlt_info);

    void NotifyScavengerCmd(const FDS_ProtocolInterface::FDSP_MsgHdrType& msg_hdr,
                            const FDS_ProtocolInterface::FDSP_ScavengerType& gc_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyScavengerCmd(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                            FDS_ProtocolInterface::FDSP_ScavengerTypePtr& gc_info);
};

}  // namespace fds

#endif  // SOURCE_LIB_OMGRCLIENT_H_

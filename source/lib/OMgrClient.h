/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef  SOURCE_LIB_OMGRCLIENT_H_
#define  SOURCE_LIB_OMGRCLIENT_H_
#include <fds_error.h>
#include <fds_volume.h>

#include "fdsp/FDSP_types.h"
#include "fdsp/FDSP_OMControlPathReq.h"
#include <util/Log.h>

#include <unordered_map>
#include <concurrency/RwLock.h>
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
struct FDSP_DltCloseType;
using FDSP_DltCloseTypePtr = boost::shared_ptr<FDSP_DltCloseType>;
}  // namespace FDS_ProtocolInterface

template <class A, class B, class C> class netClientSessionEx;
template <class A, class B, class C> class netServerSessionEx;
typedef netClientSessionEx<FDSP_OMControlPathReqClient,
                           FDSP_OMControlPathRespProcessor,
                           FDSP_OMControlPathRespIf> netOMControlPathClientSession;

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

// Callback for DMT close
typedef std::function<void(Error &err)> DmtCloseCb;

typedef void (*migration_event_handler_t)(bool dlt_type);
typedef void (*dltclose_event_handler_t)(FDSP_DltCloseTypePtr& dlt_close,
                                         const std::string& session_uuid);
typedef void (*node_event_handler_t)(int node_id,
                                     unsigned int node_ip_addr,
                                     int node_state,
                                     fds_uint32_t node_port,
                                     FDSP_MgrIdType node_type);
typedef void (*bucket_stats_cmd_handler_t)(const FDSP_MsgHdrTypePtr& rx_msg,
                                           const FDSP_BucketStatsRespTypePtr& buck_stats);

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
    bucket_stats_cmd_handler_t bucket_stats_cmd_hdlr;

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

    NodeUuid getUuid() const;
    FDSP_MgrIdType getNodeType() const;

    int registerEventHandlerForMigrateEvents(migration_event_handler_t migrate_event_hdlr);
    int registerEventHandlerForDltCloseEvents(dltclose_event_handler_t dltclose_event_hdlr);
    int registerBucketStatsCmdHandler(bucket_stats_cmd_handler_t cmd_hdlr);

    // This logging is public for external plugins.  Avoid making this object
    // too big and all methods uses its data as global variables with big lock.
    //
    fds_log        *omc_log;

    // int subscribeToOmEvents(unsigned int om_ip_addr,
    // int tennant_id, int domain_id, int omc_port_num= 0);
    int registerNodeWithOM(Platform *plat);
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
    void setCurrentDLTClosed();
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
    fds_bool_t hasCommittedDMT() const;
    int testBucket(const std::string& bucket_name,
                   const FDS_ProtocolInterface::FDSP_VolumeDescTypePtr& vol_info,
                   fds_bool_t attach_vol_reqd,
                   const std::string& accessKeyId,
                   const std::string& secretAccessKey);

    int recvMigrationEvent(bool dlt_type);
    Error updateDlt(bool dlt_type, std::string& dlt_data);

    Error updateDmt(bool dmt_type, std::string& dmt_data);
};

}  // namespace fds

#endif  // SOURCE_LIB_OMGRCLIENT_H_

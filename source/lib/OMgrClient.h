/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef  SOURCE_LIB_OMGRCLIENT_H_
#define  SOURCE_LIB_OMGRCLIENT_H_
#include <fds_error.h>
#include <fds_volume.h>

#include "fdsp/FDSP_types.h"
#include <util/Log.h>

#include <unordered_map>
#include <concurrency/RwLock.h>
#include <dlt.h>
#include <fds_dmt.h>
#include <fdsp/OMSvc.h>

#include <string>
using namespace FDS_ProtocolInterface; // NOLINT

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

    fds_rwlock omc_lock;  // to protect node_map

    node_event_handler_t node_evt_hdlr;
    migration_event_handler_t migrate_evt_hdlr;
    dltclose_event_handler_t dltclose_evt_hdlr;
    bucket_stats_cmd_handler_t bucket_stats_cmd_hdlr;

    void initOMMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr);

  public:
    OMgrClient(fpi::FDSP_MgrIdType node_type,
               const std::string& node_name,
               fds_log *parent_log);
    void setNoNetwork(bool fNoNetwork) {
        this->fNoNetwork = fNoNetwork;
    }
    bool getNoNetwork() {
    	return (this->fNoNetwork);
    }
    ~OMgrClient();

    NodeUuid getUuid() const;
    FDSP_MgrIdType getNodeType() const;

    int registerEventHandlerForMigrateEvents(migration_event_handler_t migrate_event_hdlr);
    int registerEventHandlerForDltCloseEvents(dltclose_event_handler_t dltclose_event_hdlr);
    int registerBucketStatsCmdHandler(bucket_stats_cmd_handler_t cmd_hdlr);

    // This logging is public for external plugins.  Avoid making this object
    // too big and all methods uses its data as global variables with big lock.
    //
    fds_log        *omc_log;

    int sendMigrationStatusToOM(const Error& err);

    fds_uint64_t getDltVersion();
    fds_uint32_t getLatestDlt(std::string& dlt_data);
    DltTokenGroupPtr getDLTNodesForDoidKey(const ObjectID &objId);
    const DLT* getCurrentDLT();
    void setCurrentDLTClosed();
    const DLT* getPreviousDLT();
    const TokenList& getTokensForNode(const NodeUuid &uuid) const;

    DLTManagerPtr getDltManager() { return dltMgr; }
    DMTManagerPtr getDmtManager() { return dmtMgr; }

    /**
     * Retrieves the latest DMT from OM and updates the service's instance of DMT
     */
    Error getDMT();

    /**
     * Retrieves the latest DLT from OM and updates the service's instance of DMT
     */
    Error getDLT();

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
    int getVolumeDescriptor(const std::string& volume_name);

    int recvMigrationEvent(bool dlt_type);

    Error updateDlt(bool dlt_type, std::string& dlt_data, OmDltUpdateRespCbType cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data);
};

}  // namespace fds

#endif  // SOURCE_LIB_OMGRCLIENT_H_

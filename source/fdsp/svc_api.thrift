/*
 * Copyright 2014-2015 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "svc_types.thrift"

include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.svc

/* ------------------------------------------------------------
   Operations on Services
   ------------------------------------------------------------*/

/**
 * Notify of update to DLT.
 */
struct CtrlNotifyDLTUpdate {
  1: svc_types.FDSP_DLT_Data_Type   dlt_data;
  2: i64                            dlt_version;
}

/**
 * Notifies that about a new, but not yet active DMT.
 * TODO(Andrew): This needs a response structure.
 */
struct CtrlNotifyDMTUpdate {
     1: svc_types.FDSP_DMT_Data_Type    dmt_data;
     2: i32                             dmt_version;
}

/**
 * Notify addition of volume.
 */
struct CtrlNotifyVolAdd {
  /** Volume settings and attributes. */
  1: svc_types.FDSP_VolumeDescType vol_desc;
  /**  */
  2: svc_types.FDSP_NotifyVolFlag    vol_flag;
}

/**
 * Notify removal of volume.
 */
struct CtrlNotifyVolRemove {
  /** Volume settings and attributes. */
  1: svc_types.FDSP_VolumeDescType vol_desc;
  2: svc_types.FDSP_NotifyVolFlag    vol_flag;
}

/**
 * Notify modification to volume.
 */
struct CtrlNotifyVolMod {
  /** Volume settings and attributes. */
  1: svc_types.FDSP_VolumeDescType vol_desc;
  2: svc_types.FDSP_NotifyVolFlag    vol_flag;
}

/**
 * Notify snapshot of volume.
 */
struct CtrlNotifySnapVol {
  /** Volume settings and attributes. */
  1: svc_types.FDSP_VolumeDescType vol_desc;
  2: svc_types.FDSP_NotifyVolFlag  vol_flag;
}

/**
 * A pull model similar to getDLT/getDMT that asks OM to send a list
 * of all the volume descriptors. This msg is simply an "ask"
 * and the response will contain the list of volumes.
 * Note: Similar to above, right now these are all NotifyVolAdd
 */
struct GetAllVolumeDescriptors {
    1: list<CtrlNotifyVolAdd> volumeList;
}

/**
 * Debug message for hybrid tiering
 */
struct CtrlStartHybridTierCtrlrMsg
{
}

/**
 * Request for getting the service map
 */
struct GetSvcMapMsg {
}

/**
 * Reponse for GetSvcMap request
 */
struct GetSvcMapRespMsg {
  1: required list<svc_types.SvcInfo>       svcMap;
}

/**
 * Message for requesting service status
 */
struct GetSvcStatusMsg {
}

/**
 * Response message for service status
 */
struct GetSvcStatusRespMsg {
  1: svc_types.ServiceStatus  status;
}

/**
 * Empty msg (No-op)
 */
struct EmptyMsg {
}

/**
 * Message from OM to a service to prepare for shutdown
 * The service receiving the message should stop receiving IO,
 * drain the IO and sync its state to persistent storage (if needed
 * to be persisted).
 */
struct PrepareForShutdownMsg {
}

/**
 * Message to sent to update the service information
 */
struct UpdateSvcMapMsg {
  1: required list<svc_types.SvcInfo>       updates;
}

struct GenericCommandMsg {
  1: string command;
  2: string arg;
}

struct GenericCommandRespMsg {
  1: string data;
}


/* ------------------------------------------------------------
   Common Services
   ------------------------------------------------------------*/

service PlatNetSvc {
    oneway void asyncReqt(1: svc_types.AsyncHdr asyncHdr, 2: binary payload);
    oneway void asyncResp(1: svc_types.AsyncHdr asyncHdr, 2: binary payload);
    /**
     * @brief Returns service map
     *
     * @param nullarg
     *
     * @return
     */
    list<svc_types.SvcInfo> getSvcMap(1: i64 nullarg);
    oneway void notifyNodeActive(1: svc_types.FDSP_ActivateNodeType info);

    list<svc_types.NodeInfoMsg> notifyNodeInfo(1: svc_types.NodeInfoMsg info, 2: bool bcast);
    svc_types.DomainNodes getDomainNodes(1: svc_types.DomainNodes dom);

    svc_types.ServiceStatus getStatus(1: i32 nullarg);
    map<string, i64> getCounters(1: string id);
    void resetCounters(1: string id);
    map<string, string> getStateInfo(1: string id);
    void setConfigVal(1:string key, 2:string value);
    void setFlag(1:string id, 2:i64 value);
    i64 getFlag(1:string id);
    map<string, i64> getFlags(1: i32 nullarg);
    map<string, string> getConfig(1: i32 nullarg);
    map<string, string> getProperties(1: i32 nullarg);
    string getProperty(1: string name);
    /* For setting fault injection.
     * @param cmdline format based on libfiu
     */
    bool setFault(1: string cmdline);
}

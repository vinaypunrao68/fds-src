/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
include "FDSP.thrift"
namespace cpp FDS_ProtocolInterface

/*
 * Service/domain identifiers.
 */
struct SvcUuid {
    1: required i64           svc_uuid,
}

struct SvcID {
    1: required SvcUuid       svc_uuid,
    2: required string        svc_name,
}

struct SvcVer {
    1: required i16           ver_major,
    2: required i16           ver_minor,
}

struct DomainID {
    1: required SvcUuid       domain_id,
    2: required string        domain_name,
}

/*
 * Generic response message format.
 */
 // TODO(Rao): Not sure if it's needed..Remove
struct RespHdr {
    1: required i32           status,
    2: required string        text,
}

/*
 * This message header is owned, controlled and set by the net service layer.
 * Application code treats it as opaque type.
 */
struct AsyncHdr {
    1: required i32           	msg_chksum,
    2: required i32           	msg_src_id,
    3: required SvcUuid       	msg_src_uuid,
    4: required SvcUuid       	msg_dst_uuid,
    5: required i32           	msg_code,
}

/*
 * SM service specific messages
 */
struct GetObjectMsg {
   1: required AsyncHdr        	hdr;
   2: i64    			volume_id,
   3: FDSP.FDS_ObjectIdType 	data_obj_id,
}

struct GetObjectResp {
   1: required AsyncHdr        	hdr;
   2: i32              		data_obj_len,
   3: binary           		data_obj,
}

struct PutObjectMsg {
   1: required AsyncHdr        	hdr;
   2: i64    			volume_id;
   3: i64                      	origin_timestamp;
   4: FDSP.FDS_ObjectIdType 	data_obj_id;
   5: i32                      	data_obj_len;
   6: binary                   	data_obj;
}

struct PutObjectRspMsg {
   1: required AsyncHdr        	hdr;
}

/*
 * DM service specific messages
 */
struct QueryCatalogMsg {
   1: required AsyncHdr        	hdr;
   2: i64    			volume_id;
   3: string   			blob_name;		/* User visible name of the blob*/
   4: i64 			blob_version;        	/* Version of the blob to query */
   5: i64 			blob_size;
   6: i32 			blob_mime_type;
   7: FDSP.FDSP_BlobDigestType 	digest;
   8: FDSP.FDSP_BlobObjectList 	obj_list; 		/* List of object ids of the objects that this blob is being mapped to */
   9: FDSP.FDSP_MetaDataList 	meta_list;		/* sequence of arbitrary key/value pairs */

   10: i32      		dm_transaction_id;   	/* Transaction id */
   11: i32      		dm_operation;        	/* Transaction type = OPEN, COMMIT, CANCEL */
}

struct UpdateCatalogMsg {
   1: required AsyncHdr        	hdr;
   2: i64    			volume_id;
   3: string 			blob_name; 	/* User visible name of the blob */
   4: i64                       blob_version; 	/* Version of the blob */
   5: FDSP.TxDescriptor 	txDlsc; 	/* Transaction ID...can supersede other tx fields */
   6: FDSP.FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
}

struct UpdateCatalogRspMsg {
   1: required AsyncHdr       	hdr;
}

struct SetBlobMetaDataMsg {
   1: required AsyncHdr        	hdr;
   2: string 			volumeName;
   3: string 			blobName;
   4: FDSP.FDSP_MetaDataList 	metaDataList;
}

struct SetBlobMetaDataRspMsg {
   1: required AsyncHdr        	hdr;
}

/*
 * --------------------------------------------------------------------------------
 * Node endpoint/service registration handshake
 * --------------------------------------------------------------------------------
 */

/*
 * Uuid to physical location binding registration.
 */
struct UuidBindMsg {
    1: required AsyncHdr                 header,
    2: required SvcID                    svc_id,
    3: required string                   svc_addr,
    4: required i32                      svc_port,
    5: required SvcID                    svc_node,
    6: required string                   svc_auto_name,
    7: required FDSP.FDSP_MgrIdType      svc_type,
}

/*
 * Node storage capability message format.
 */
struct StorCapMsg {
    1: i32                    disk_iops_max,
    2: i32                    disk_iops_min,
    3: double                 disk_capacity,
    4: i32                    disk_latency_max,
    5: i32                    disk_latency_min,
    6: i32                    ssd_iops_max,
    7: i32                    ssd_iops_min,
    8: double                 ssd_capacity,
    9: i32                    ssd_latency_max,
    10: i32                   ssd_latency_min,
    11: i32                   ssd_count,
    12: i32                   disk_type,
    13: i32                   disk_count,
}

enum NodeSvcMask {
    NODE_SVC_SM       = 0x0001,
    NODE_SVC_DM       = 0x0002,
    NODE_SVC_AM       = 0x0004,
    NODE_SVC_OM       = 0x0008,
    NODE_SVC_GENERAL  = 0x1000
}

/*
 * Node registration message format.
 */
struct NodeInfoMsg {
    1: required UuidBindMsg   node_loc,
    2: required DomainID      node_domain,
    3: required StorCapMsg    node_stor,
    4: required i32           nd_base_port,
    5: required i32           nd_svc_mask,
}

enum ServiceStatus {
    SVC_STATUS_INVALID      = 0x0000,
    SVC_STATUS_ACTIVE       = 0x0001,
    SVC_STATUS_INACTIVE     = 0x0002,
    SVC_STATUS_IN_ERR       = 0x0004
}

/**
 * Service map info
 */
struct SvcInfo {
    1: required SvcID                    svc_id,
    2: required i32                      svc_port,
    3: required FDSP.FDSP_MgrIdType      svc_type,
    4: required ServiceStatus            svc_status,
    5: required string                   svc_auto_name,
}

struct NodeSvcInfo {
    1: required SvcUuid                  node_base_uuid,
    2: i32                               node_base_port,
    3: string                            node_addr,
    4: string                            node_auto_name,
    5: FDSP.FDSP_NodeState               node_state;
    6: i32                               node_svc_mask,
    7: list<SvcInfo>                     node_svc_list,
}

struct DomainNodes {
    1: required AsyncHdr                 dom_hdr,
    2: required DomainID                 dom_id,
    3: list<NodeSvcInfo>                 dom_nodes,
}

/*
 * --------------------------------------------------------------------------------
 * Common services
 * --------------------------------------------------------------------------------
 */

service BaseAsyncSvc {
    oneway void asyncReqt(1: AsyncHdr header, 2: string payload),
    oneway void asyncResp(1: AsyncHdr header, 2: string payload),
    RespHdr uuidBind(1: UuidBindMsg msg),
}

service PlatNetSvc extends BaseAsyncSvc {
    oneway void allUuidBinding(1: UuidBindMsg mine),
    oneway void notifyNodeActive(1: FDSP.FDSP_ActivateNodeType info),

    list<NodeInfoMsg> notifyNodeInfo(1: NodeInfoMsg info, 2: bool bcast),
    DomainNodes getDomainNodes(1: DomainNodes dom),

    // ServiceStatus getStatus(1: i32 nullarg),
    // map<string, i64> getCounters(1: string id),
    // void setConfigVal(1:string id, 2:i64 value )
    ServiceStatus getStatus(1: i32 nullarg),
    map<string, i64> getCounters(1: string id),
    void setConfigVal(1:string id, 2:i64 value )
    void setFlag(1:string id, 2:i64 value )
    i64 getFlag(1:string id)
    map<string, i64> getFlags(1: i32 nullarg)
}

service SMSvc extends PlatNetSvc {
    oneway void getObject(1: GetObjectMsg getObjMsg);
    oneway void putObject(1: PutObjectMsg putObjMsg);
}

service DMSvc extends BaseAsyncSvc {
    oneway void queryCatalogObject(1:QueryCatalogMsg queryMsg);
    oneway void updateCatalog(1:UpdateCatalogMsg updCatMsg);
}

service AMSvc extends BaseAsyncSvc {
}

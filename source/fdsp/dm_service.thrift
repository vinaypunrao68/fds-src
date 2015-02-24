/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "fds_service.thrift"
include "FDSP.thrift"
include "common.thrift"
include "snapshot.thrift"

namespace cpp FDS_ProtocolInterface

/**
 * DM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service DMSvc extends fds_service.PlatNetSvc {
}

/* Abort Blob  Transaction  request message */
struct AbortBlobTxMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   5: i64			txId;
}

/* Abort Blob traction response message */
struct AbortBlobTxRspMsg {
}

/* Commit Blob  Transaction  request message */
struct CommitBlobTxMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   4: i64 			txId;
   5: i64                       dmt_version,
}

/* Commit Blob traction response message */
struct CommitBlobTxRspMsg {
   1: i64                       byteCount;  /* Blob size */
   2: FDSP.FDSP_MetaDataList    meta_list;  /* sequence of arbitrary key/value pairs */
}

/* snapshot message From OM => DM */
struct CreateSnapshotMsg {
    1:snapshot.Snapshot snapshot
}  

struct CreateSnapshotRespMsg {
    1:i64 snapshotId
}

struct CreateVolumeCloneMsg {
     1:i64 volumeId,
     2:i64 cloneId,
     3:string cloneName,
     4:i64 VolumePolicyId
}  

struct CreateVolumeCloneRespMsg {
     1:i64 cloneId
}

struct DeleteBlobMsg {
  1: i64                       txId;
  2: i64                       volume_id;
  3: string                    blob_name;
  4: i64                       blob_version;
}

struct DeleteBlobRspMsg {
}

/* delete catalog object Transaction  request message */
struct DeleteCatalogObjectMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
}

struct DeleteCatalogObjectRspMsg {
}

struct DeleteSnapshotMsg {
    1:i64 snapshotId
}

struct DeleteSnapshotRespMsg {
    1:i64 snapshotId
}

struct ForwardCatalogMsg {
   1: i64    			volume_id;
   2: string 			blob_name; 	/* User visible name of the blob */
   3: i64                       blob_version; 	/* Version of the blob */
   4: FDSP.FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
   5: FDSP.FDSP_MetaDataList 	meta_list;      /* sequence of arbitrary key/value pairs */
}

/* Forward catalog update response message */
struct ForwardCatalogRspMsg {
}

struct GetBlobMetaDataMsg {
  1: i64                       volume_id;
  2: string                    blob_name;
  3: i64                       blob_version;
  4: i64                       byteCount;
  5: FDSP.FDSP_MetaDataList    metaDataList;
}

struct GetBlobMetaDataRspMsg {
}

struct GetBucketMsg {
  //request
  1: required i64              volume_id;
  2: i64                       startPos = 0;  
  3: i64                       count = 10000;
  4: string                    pattern = "";
  5: common.BlobListOrder      orderBy = 0;
  6: bool                      descending = false;
}

struct GetBucketRspMsg {
  //response
  1: required common.BlobDescriptorListType     blob_descr_list;
}

struct GetDmStatsMsg {
  1: i64                       volume_id;
  // response
  2: i64                       commitlog_size;
  3: i64                       extent0_size;
  4: i64                       extent_size;
  5: i64                       metadata_size;
}

struct GetDmStatsRespMsg {
}

struct GetVolumeMetaDataMsg {
  1: i64                        volume_id;
  //response
  2: FDSP.FDSP_VolumeMetaData   volume_meta_data;
}

struct GetVolumeMetaDataRspMsg {
}

/*
 * --------------------------------------------------------------------------------
 * DM service specific messages
 * --------------------------------------------------------------------------------
 */
 /* Query catalog request message */
struct QueryCatalogMsg {
   1: i64    			volume_id;
   2: string   			blob_name;		/* User visible name of the blob*/
   3: i64               start_offset;   /* Starting offset into the blob */
   4: i64               end_offset;     /* End offset into the blob */
   5: i64 		blob_version;   /* Version of the blob to query */
   6: FDSP.FDSP_BlobObjectList 	obj_list; 		/* List of object ids of the objects that this blob is being mapped to */
   7: FDSP.FDSP_MetaDataList 	meta_list;		/* sequence of arbitrary key/value pairs */
   8: i64                       byteCount;  /* Blob size */
}

// TODO(Rao): Use QueryCatalogRspMsg.  In current implementation we are using QueryCatalogMsg
// for response as well
/* Query catalog request message */
struct QueryCatalogRspMsg {
}

struct ReloadVolumeMsg {
    1:i64 volId
}

struct ReloadVolumeRspMsg {
}

/* get the list of blobs in volume Transaction  request message */
struct SetBlobMetaDataMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   4: FDSP.FDSP_MetaDataList    metaDataList; 
   5: i64                   	txId;
}

/* Set blob metadata request message */
struct SetBlobMetaDataRspMsg {
}

/* Start Blob  Transaction  request message */
struct StartBlobTxMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   4: i32 			blob_mode;
   5: i64 			txId;
   6: i64                       dmt_version;
}

/* start Blob traction response message */
struct StartBlobTxRspMsg {
}

/* Registration for streaming stats */
struct StatStreamRegistrationMsg {
   1:i32 id,
   2:string url,
   3:string method
   4:fds_service.SvcUuid dest,
   5:list<i64> volumes,
   6:i32 sample_freq_seconds,
   7:i32 duration_seconds,
}

struct StatStreamRegistrationRspMsg {
}

struct StatStreamDeregistrationMsg {
    1: i32 id;
}

struct StatStreamDeregistrationRspMsg {
}

/* Update catalog request message */
struct UpdateCatalogMsg {
   1: i64    			volume_id;
   2: string 			blob_name; 	/* User visible name of the blob */
   3: i64                       blob_version; 	/* Version of the blob */
   4: i64                   	txId;
   5: FDSP.FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
}

/* Update catalog response message */
struct UpdateCatalogRspMsg {
}

/* Update catalog once request message */
struct UpdateCatalogOnceMsg {
   1: i64    			volume_id;
   2: string 			blob_name; 	/* User visible name of the blob */
   3: i64                       blob_version; 	/* Version of the blob */
   4: i32 			blob_mode;
   5: i64                       dmt_version;
   6: i64                   	txId;
   7: FDSP.FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
   8: FDSP.FDSP_MetaDataList 	meta_list;	/* sequence of arbitrary key/value pairs */
}

/* Update catalog once response message */
struct UpdateCatalogOnceRspMsg {
   1: i64                       byteCount;  /* Blob size */
   2: FDSP.FDSP_MetaDataList    meta_list;  /* sequence of arbitrary key/value pairs */
}

/* Volume sync state msg sent from source to destination DM */
struct VolSyncStateMsg
{
    1: i64        volume_id;
    2: bool       forward_complete;   /* true = forwarding done, false = second rsync done */
}

struct VolSyncStateRspMsg {
}

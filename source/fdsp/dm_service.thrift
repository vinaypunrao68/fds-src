/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "fds_service.thrift"
include "FDSP.thrift"
include "common.thrift"
include "snapshot.thrift"
include "pm_service.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.dm

/* -------------------- DM Variable Types -------------------- */

/**
 * Metadata information for a volume.
 */
struct FDSP_VolumeMetaData {
  /** The number of blobs in the volume */
  1: i64 blobCount;
  /** The total logical capacity consumed by all blobs the volume */
  2: i64 size;
  /**
   * The total number of objects in all blobs in the volume.
   * TODO(Andrew): Do we need this field? Seems kinda useless...
   */
  3: i64 objectCount;
}

/**
 * A metadata key-value pair. The key and value have no explicit size
 * restriction.
 */
struct FDSP_MetaDataPair {
 1: string key,
 2: string value,
}

/**
 * A list of metadata key-value pairs. The order of the list
 * is arbitrary.
 * TODO(Andrew): Is a set better since there's no order and
 * keys should be unique?
 */
typedef list <FDSP_MetaDataPair> FDSP_MetaDataList

/**
 * A list of blob descriptors. The list may be ordered if
 * requested.
 */
typedef list<common.BlobDescriptor> BlobDescriptorListType

/**
 * A list of volume IDs.
 * TODO(Andrew): This should probably be in the common
 * definition and should have a better name.
 */
typedef list<i64> vol_List_Type

/**
 * Describes a group of volume IDs involved in
 * a replica migration to a specific DM.
 */
struct FDSP_metaData
{
    1: vol_List_Type  volList;
    2: common.SvcUuid  node_uuid;
}

/**
 * Describes a set of migration events between one or more DMs.
 * TODO(Andrew): Should be a set since migrations should be unique.
 */
typedef list<FDSP_metaData> FDSP_metaDataList

struct FDSP_BlobObjectInfo {
 1: i64 offset,
 2: FDSP.FDS_ObjectIdType data_obj_id,
 3: i64 size
 4: bool blob_end;
}

/**
 * List of blob offsets and their corresponding object IDs.
 * TODO(Andrew): Should be a set since blob offsets should
 * be unique.
 */
typedef list<FDSP_BlobObjectInfo> FDSP_BlobObjectList

/* -------------------- Operations on Volumes --------------------*/

/**
 * Lists specific contents of a volume. A specific offset into
 * the current list of blobs in the volumes and number of blobs
 * to be returned may be set.
 * Note that each call is indendant, so the order and contents
 * may change between calls.
 * A pattern filter may be specified that returns only blobs whose
 * name matches the string pattern.
 */
struct GetBucketMsg {
  1: required i64              volume_id;
  2: i64                       startPos = 0;  
  3: i64                       count = 10000;
  4: string                    pattern = "";
  5: common.BlobListOrder      orderBy = 0;
  6: bool                      descending = false;
}
/**
 * Returns a list of blob descriptors matching the query. The
 * list may be ordered depending on the query.
 */
struct GetBucketRspMsg {
  1: required BlobDescriptorListType     blob_descr_list;
}

/**
 * Returns a
 */
struct GetVolumeMetaDataMsg {
  1: i64                        volume_id;
  2: FDSP_VolumeMetaData        volume_meta_data;
}
struct GetVolumeMetaDataRspMsg {
}

/**
 * Creates a snapshot of the volume. A snapshot is a read-only,
 * point-in-time copy of the voluem.
 * TODO(Andrew): Why not remove the snapshot struct and just
 * add those structures to this message?
 */
struct CreateSnapshotMsg {
    1:snapshot.Snapshot snapshot
}
/**
 * Response contains the ID of the newly created snapshot.
 * If the snapshot could not be taken, ERR_DM_SNAPSHOT_FAILED
 * is returned.
 */
struct CreateSnapshotRespMsg {
    1:i64 snapshotId
}

struct DeleteSnapshotMsg {
    1:i64 snapshotId
}
struct DeleteSnapshotRespMsg {
    1:i64 snapshotId
}

/**
 * Creates a clone of the volume. A clone is an entirely
 * new, read/write-able volume that contains identical contents
 * to the volume it is cloned from. Once cloned updates to
 * the original and clone volume are unrelated.
 */
struct CreateVolumeCloneMsg {
     1:i64 volumeId,
     2:i64 cloneId,
     3:string cloneName,
     4:i64 VolumePolicyId
}  
/**
 * Response contains the ID of the newly created clone
 * volume. This ID is equivalent to the volume's ID.
 * If the clone could not be created, ERR_I_DONT_KNOW (?)
 * is returned.
 */
struct CreateVolumeCloneRespMsg {
     1:i64 cloneId
}

struct ReloadVolumeMsg {
    1:i64 volId
}
struct ReloadVolumeRspMsg {
}

/* -------------------- Operations on Blobs -------------------- */

/**
 * Start Blob  Transaction  request message
 */
struct StartBlobTxMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   4: i32 			blob_mode;
   5: i64 			txId;
   6: i64                       dmt_version;
}
/**
 * start Blob traction response message
 */
struct StartBlobTxRspMsg {
}

/**
 * Commits a currently active transaction.
 */
struct CommitBlobTxMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   4: i64 			txId;
   5: i64                       dmt_version,
}
/**
 * Response contains the logical size of the blob and its
 * metadata after the the transaction is committed.
 * If the transaction did not exist, ERR_DM_INVALID_TX_ID is
 * returned.
 * If the commit failed because the transaction contained invalid
 * updates, ERR_I_HAVE_NO_CLUE (?) is returned.
 */
struct CommitBlobTxRspMsg {
   /** Blob size */
   1: i64                  byteCount;
   /** Sequence of arbitrary key/value pairs */
   2: FDSP_MetaDataList    meta_list;
}

/**
 * Aborts a currently active transaction.
 */
struct AbortBlobTxMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   5: i64			txId;
}
/**
 * Abort Blob traction response message
 */
struct AbortBlobTxRspMsg {
}

/**
 * Update catalog request message
 */
struct UpdateCatalogMsg {
   1: i64    			volume_id;
   2: string 			blob_name; 	/* User visible name of the blob */
   3: i64                       blob_version; 	/* Version of the blob */
   4: i64                   	txId;
   5: FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
}
/**
 * Update catalog response message
 */
struct UpdateCatalogRspMsg {
}

/**
 * Update catalog once request message
 */
struct UpdateCatalogOnceMsg {
   1: i64    			volume_id;
   2: string 			blob_name; 	/* User visible name of the blob */
   3: i64                       blob_version; 	/* Version of the blob */
   4: i32 			blob_mode;
   5: i64                       dmt_version;
   6: i64                   	txId;
   7: FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
   8: FDSP_MetaDataList 	meta_list;	/* sequence of arbitrary key/value pairs */
}
/**
 * Update catalog once response message
 */
struct UpdateCatalogOnceRspMsg {
   1: i64                       byteCount;  /* Blob size */
   2: FDSP_MetaDataList         meta_list;  /* sequence of arbitrary key/value pairs */
}

struct DeleteBlobMsg {
  1: i64                       txId;
  2: i64                       volume_id;
  3: string                    blob_name;
  4: i64                       blob_version;
}
struct DeleteBlobRspMsg {
}

/**
 * get the list of blobs in volume Transaction  request message
 */
struct SetBlobMetaDataMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   4: FDSP_MetaDataList         metaDataList;
   5: i64                   	txId;
}
/* Set blob metadata request message */
struct SetBlobMetaDataRspMsg {
}

struct GetBlobMetaDataMsg {
  1: i64                       volume_id;
  2: string                    blob_name;
  3: i64                       blob_version;
  4: i64                       byteCount;
  5: FDSP_MetaDataList         metaDataList;
}
struct GetBlobMetaDataRspMsg {
}

/**
 * Query catalog request message
 */
struct QueryCatalogMsg {
   1: i64    			volume_id;
   2: string   			blob_name;		/* User visible name of the blob*/
   3: i64               start_offset;   /* Starting offset into the blob */
   4: i64               end_offset;     /* End offset into the blob */
   5: i64 		blob_version;   /* Version of the blob to query */
   6: FDSP_BlobObjectList 	obj_list; 		/* List of object ids of the objects that this blob is being mapped to */
   7: FDSP_MetaDataList 	meta_list;		/* sequence of arbitrary key/value pairs */
   8: i64                       byteCount;  /* Blob size */
}
/**
 * Query catalog request message
 */
struct QueryCatalogRspMsg {
}

/* -------------------- Operations for Statistics --------------------*/

/* Registration for streaming stats */
struct StatStreamRegistrationMsg {
   1:i32 id,
   2:string url,
   3:string method
   4:common.SvcUuid dest,
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

/**
 * time slot containing volume stats
 */
struct VolStatSlot {
    1: i64    rel_seconds;    // timestamp in seconds relative to start_timestamp
    2: binary slot_data;      // represents time slot of stats
}

/**
 * Volume's time-series of stats
 */
struct VolStatList {
    1: i64                  volume_id;  // volume id
    2: list<VolStatSlot>    statlist;   // list of time slots
}

/**
 * Stats from a service to DM for aggregation
 */
struct StatStreamMsg {
    1: i64                    start_timestamp;
    2: list<VolStatList>      volstats;
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

/* -------------------- Operations for Replication --------------------*/

/* DM meta data migration request sent to source DM */
struct CtrlDMMigrateMeta
{
     /* meta data */
     2: FDSP_metaDataList          metaVol;
}

struct CtrlNotifyDMTClose {
     1: FDSP.FDSP_DmtCloseType    dmt_close;
}

struct CtrlNotifyDMTUpdate {
     1: FDSP.FDSP_DMT_Type        dmt_data;
     2: i32                       dmt_version;
}

struct CtrlNotifyDMAbortMigration {
     1: i64  DMT_version;
}

struct ForwardCatalogMsg {
   1: i64    			volume_id;
   2: string 			blob_name; 	/* User visible name of the blob */
   3: i64                       blob_version; 	/* Version of the blob */
   4: FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
   5: FDSP_MetaDataList 	meta_list;      /* sequence of arbitrary key/value pairs */
}
/**
 * Forward catalog update response message
 */
struct ForwardCatalogRspMsg {
}

/* Volume sync state msg sent from source to destination DM */
struct VolSyncStateMsg
{
    1: i64        volume_id;
    2: bool       forward_complete;   /* true = forwarding done, false = second rsync done */
}
struct VolSyncStateRspMsg {
}

/* -------------------- Other specified services -------------------- */

/**
 * DM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service DMSvc extends pm_service.PlatNetSvc {
}

/**
 * Some service layer test thingy.
 */
service TestDMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void updateCatalog(1: fds_service.AsyncHdr asyncHdr, 2: UpdateCatalogOnceMsg payload);
}

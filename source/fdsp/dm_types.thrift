/*
 * Copyright 2015 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp
 */

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.dm.types

include "common.thrift"

/**
 * Volume status information.
 */
struct VolumeStatus {
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
typedef list<FDSP_MetaDataPair> FDSP_MetaDataList

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

/**
 * Describes a blob offset's information.
 */
struct FDSP_BlobObjectInfo {
 1: i64 offset,
 2: common.FDS_ObjectIdType data_obj_id,
 3: i64 size
}

/**
 * List of blob offsets and their corresponding object IDs.
 * TODO(Andrew): Should be a set since blob offsets should
 * be unique.
 */
typedef list<FDSP_BlobObjectInfo> FDSP_BlobObjectList

/**
 * Describes the DMT being closed.
 * TODO(Andrew): Why have a type with 1 field? The API should just include
 * the DMT version.
 */
struct FDSP_DmtCloseType {
  1: i64 DMT_version
}

/**
 * Describes a DMT, its version, table, and type.
 * TODO(Andrew): Why do we have this? The version is part of the serialized
 * DMT and WTF is type?
 */
struct FDSP_DMT_Type {
  1: i64 dmt_version,
  /** DMT table + meta in binary format */
  2: binary dmt_data,
  3: bool dmt_type,
}

/**
 * Used for OM to tell DM to pull volume descriptors from other DMs.
 */
struct DMVolumeMigrationGroup {
  1:  common.SvcUuid          source,
  2:  list<common.FDSP_VolumeDescType> VolDescriptors,
}

/**
 * Used for volume migration between DMs
 */
enum DMVolumeMigrationDiffType {
  FDSP_BLOB_MODIFIED,  // Blob is modified
  FDSP_BLOB_ADDED,     // Blob is added into source but missing on destination
  FDSP_BLOB_DELETED,   // Blob present on dest but deleted in source
}

struct DMBlobDescriptorDiff {
  1: DMVolumeMigrationDiffType type,
  2: BlobDescriptorListType blob_descr_list;
}

struct DMVolumeMigrationDiff {
  1: i64                        volume_id;
  2: i32                        status;
  3: list<DMBlobDescriptorDiff> blob_diff_list;
}

struct DMMigrationObjListDiff {
  1: string               blob_name;
  2: FDSP_BlobObjectList  blob_diff_list;
}

/**
 * blob Id and blob  descriptor  pairs send  from Source DM 
 * to destination DM.  This message is overloaded to migrate
 * volume descriptor. second filed beeing string , we will have to send
 * the serialized verion of the descriptors. 
 */
struct DMBlobDescListDiff {
  1: string                     vol_blob_name,
  2: string                     vol_blob_desc;
}

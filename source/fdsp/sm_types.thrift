/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "svc_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.sm.types

/* ------------------------------------------------------------
   StorMgr Enumerations
   ------------------------------------------------------------*/

/**
 * Scavenger Commands
 */
enum FDSP_ScavengerCmd {
  /** Enable Automatic Garbage Collection */
  FDSP_SCAVENGER_ENABLE;
  /** Disable Automatic Garbage Collection */
  FDSP_SCAVENGER_DISABLE;
  /** Manually Trigger Garbage Collection */
  FDSP_SCAVENGER_START;
  /** Mannually Stop Garbage Collection */
  FDSP_SCAVENGER_STOP;
}

/**
 * Scavenger Status States
 */
enum FDSP_ScavengerStatusType {
  SCAV_ACTIVE   = 1;
  SCAV_INACTIVE = 2;
  SCAV_DISABLED = 3;
  SCAV_STOPPING = 4;
}

/**
 * Scrubber Status States
 */
enum FDSP_ScrubberStatusType {
  FDSP_SCRUB_ENABLE     = 1;
  FDSP_SCRUB_DISABLE    = 2;
}

/**
 * Object reconciliation flag
 */
enum ObjectMetaDataReconcileFlags {
  /** No need to reconcile meta data. A new object. */
  OBJ_METADATA_NO_RECONCILE = 0;
  /** Need to reconcile meta data. Metadata has changed */
  OBJ_METADATA_RECONCILE    = 1;
  /** Overwrite existing metadata */
  OBJ_METADATA_OVERWRITE    = 2;
}

/**
 * Command to enable/disable, start/stop smcheck
 */
enum SMCheckCmd {
    SMCHECK_START;
    SMCHECK_STOP;
}

/**
 * Bitmap flag of curent SMCheck status
 */
enum SMCheckStatusType {
    SMCHECK_STATUS_INACTIVE;
    SMCHECK_STATUS_ACTIVE;
}

/**
 * Object store states
 */
enum FDSP_ObjectStoreState {
  OBJECTSTORE_NORMAL;
  OBJECTSTORE_READ_ONLY;
  OBJECTSTORE_UNAVAILABLE;
}

/* ------------------------------------------------------------
   StorMgr Types
   ------------------------------------------------------------*/

/**
 * Object volume association
 */
struct MetaDataVolumeAssoc {
  /** Object volume association */
  1: i64    volumeAssoc;
  /** reference count for volume association */
  2: i64    volumeRefCnt;
}

/**
 * 
 */
struct SMTokenMigrationGroup {
  /** Token source */
  1: svc_types.SvcUuid source;
  /** Tokens */
  2: list<i32>      tokens;
}

/**
 * Object + Data + MetaData to be propogated to the destination SM from source SM
 */
struct CtrlObjectMetaDataPropagate
{
  /** Object ID */
  1: svc_types.FDS_ObjectIdType        objectID;
  /** Reconcile action */
  2: ObjectMetaDataReconcileFlags   objectReconcileFlag;
  /** User data */
  3: string                         objectData;
  /** Volume information */
  4: list<MetaDataVolumeAssoc>      objectVolumeAssoc;
  /** Object refcnt */
  5: i64                            objectRefCnt;
  /** Compression type for this object */
  6: i32                            objectCompressType;
  /** Size of data after compression */
  7: i32                            objectCompressLen;
  /** Object block size */
  8: i32                            objectBlkLen;
  /** Object size */
  9: i32                            objectSize;
  /** Object flag */
  10: i32                           objectFlags;
  /** Object expieration time */
  11: i64                           objectExpireTime;
}

/**
 * Object + subset of MetaData to determine if either the object or
 * associated MetaData (subset) needs sync'ing.
 */
struct CtrlObjectMetaDataSync 
{
  /** Object ID */
  1: svc_types.FDS_ObjectIdType      objectID;
  /* RefCount of the object */
  2: i64                        objRefCnt;
  /* volume information */
  3: list<MetaDataVolumeAssoc>  objVolAssoc;
}

/**
 * Not sure...FIXME
 */
struct FDSP_DltCloseType {
  /** DLT version that started migration */
  1: i64 DLT_version;
}

/**
 * Migrations status type
 */
struct FDSP_MigrationStatusType {
  /** DLT version of migration */
  1: i64 DLT_version;
  /** ? FIXME */
  2: i32 context;
}

/**
 * Scavenger Command
 */
struct FDSP_ScavengerType {
  /** Command to scavenger. */
  1: FDSP_ScavengerCmd  cmd;
}

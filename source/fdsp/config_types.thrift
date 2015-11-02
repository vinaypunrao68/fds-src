/*
 * Copyright 2014 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "common.thrift"
include "FDSP.thrift"
include "svc_types.thrift"

namespace cpp fds.apis
namespace java com.formationds.apis

/* ------------------------------------------------------------
   Configuration Enumerations
   ------------------------------------------------------------*/

/**
 * Volume Media Policy
 */
enum MediaPolicy {
  SSD_ONLY = 0;
  HDD_ONLY = 1;
  HYBRID_ONLY = 3;
}

/**
 * Volume Storage Type
 */
enum VolumeType {
  OBJECT = 0;
  BLOCK = 1;
  ISCSI = 2;
  NFS = 3;
}

/* ------------------------------------------------------------
   Configuration Types
   ------------------------------------------------------------*/

/**
 * Snapshot and clone from CLI => OM structures 
 */
struct SnapshotPolicy {
  /** snapshot uuid */
  1: i64 id;
  /** a string representing the name of the policy, should be unique */
  2: string policyName;
  /** a string representing the recurrence rule as per RFC 2445 iCalender */
  3: string recurrenceRule;
  /** the retention time in seconds */
  4: i64 retentionTimeSeconds;
  /** the snapshot state */
  5: svc_types.ResourceState state;
  /** the timeline time in seconds */
  6: i64  timelineTime;
}

struct FDSP_ModifyVolType {
  1: string 		 vol_name,  /* Name of the volume */
  2: i64		 vol_uuid,
  3: svc_types.FDSP_VolumeDescType	vol_desc,  /* New updated volume descriptor */
}

/**
 * Statistic stream parameters.
 */
struct StreamingRegistrationMsg {
  /** the stream registeration uuid */
  1: i32 id;
  /** a string representing the URL specifing the ingest end point */
  2: string url;
  /** a list of strings representing the volume names */
  3: list<string> volume_names;
  /** the frequency of the sampling data, in minutes */
  4: i32 sample_freq_seconds;
  /** the duration at which simple data will be sent */
  5: i32 duration_seconds;
  /** a string representing HTTP method of the ingest end point */
  6: string http_method;
}

/**
 * Local Domain descriptor.
 */
struct LocalDomainDescriptor {
  /** The Local Domain uuid */
  1: required i32 id;
  /** A string representing the name of the Local Domain, i.e. domain name */
  2: required string name;
  /** A string representing the location or usage of the Local Domain. */
  3: required string site;
  /** The date created, in epoch seconds. */
  4: required i64 createTime;
  /** 'true' if the associated LocalDomain instance represents the current domain. However, once this crosses a domain boundary it can't be trusted. */
  5: required bool current;
  /** When not the current local domain, this provides OM contact information for the referenced local domain. */
  6: required FDSP.FDSP_RegisterNodeType omNode;
}

/**
 * Local Domain descriptor for interface version V07.
 */
struct LocalDomainDescriptorV07 {
  /** The Local Domain uuid */
  1: required i64 id;
  /** A string representing the name of the Local Domain, i.e. domain name */
  2: required string name;
  /** A string representing the location or usage of the Local Domain. */
  3: required string site;
}

struct FDSP_ActivateOneNodeType {
  1: i32        domain_id,
  2: svc_types.FDSP_Uuid  node_uuid,
  3: bool       activate_sm,
  4: bool       activate_dm,
  5: bool       activate_am
}

struct FDSP_RemoveServicesType {
  1: string node_name, // Name of the node that contains services
  2: svc_types.FDSP_Uuid node_uuid,  // used if node name is not provided
  3: bool remove_sm,   // true if sm needs to be removed
  4: bool remove_dm,   // true to remove dm
  5: bool remove_am    // true to remove am
}

/**
 * Tenant descriptor.
 */
struct Tenant {
  /** the tenant uuid */
  1: i64 id;
  /** a string representing the identifier of the tenant, i.e. tenant name */
  2: string identifier;
}

/**
 * A user of the system.
 */
struct User {
  /** the user uuid */
  1: i64 id;
  /** a string representing the user identifier of the user, i.e. user name */
  2: string identifier;
  /** a string representing the users password hash */
  3: string passwordHash;
  /** a string representing the secret passphrase */
  4: string secret;
  /** a boolean flag indicating if the user has administration permissions */
  5: bool isFdsAdmin;
}

struct FDSP_CreateVolType {
  1: string                  vol_name,
  2: svc_types.FDSP_VolumeDescType     vol_info, /* Volume properties and attributes */
}

struct FDSP_DeleteVolType {
  1: string 		 vol_name,  /* Name of the volume */
  // i64    		 vol_uuid,
  2: i32			 domain_id,
}

exception FDSP_VolumeNotFound {
  1: string message;
}

struct FDSP_GetVolInfoReqType {
 1: string vol_name,    /* name of the volume */
 3: i32    domain_id,
}

/**
 * Volume configuration parameters.
 */
struct VolumeSettings {
  /** the maximum object size in bytes */
  1: required i32 maxObjectSizeInBytes;
  /** the volume type */
  2: required VolumeType volumeType;
  /** the block device size in bytes */
  3: required i64 blockDeviceSizeInBytes;
  /** the continue commit log retention, in seconds */
  4: required i64 contCommitlogRetention;
  /** the media policy type */
  5: MediaPolicy mediaPolicy;
  /** the default access mode */
  6: optional common.VolumeAccessMode default_mode;
}

/**
 * Volume attributes and status
 */
struct VolumeDescriptor {
  /** the string representing the volume name, MUST be unique */
  1: required string name;
  /** the date created, in epoach seconds */
  2: required i64 dateCreated;
  /** the VolumeSettings representing the configuration parameters */
  3: required VolumeSettings policy;
  /** the administrating tenant id */
  4: required i64 tenantId;
  /** the volume uuid */
  5: i64 volId;
  /** the ResourceState representing the current state of the volume */
  6: svc_types.ResourceState   state;
}

/**
 * Subscription types. These divide into two groups: Content-oriented and transaction-oriented.
 */
enum SubscriptionType {
  /** Content is compared between primary and replica and changed content replaced. */
  CONTENT_DELTA = 0;

  /** Content changing transactions are pushed from primary to replica without the assistance of a transaction log. */
  TRANSACTION_NO_LOG = 1;

  /** Content changing transactions are pushed from primary to replica with the assistance of a transaction log. */
  TRANSACTION_LOG = 2;
}

/**
 * Subscription refresh schedule types. For content-oriented subscriptions types,
 * the replica gets refreshed from the primary according to some schedule whose
 * basis is given by one of these scheduling types.
 */
enum SubscriptionScheduleType {
  NA = 0;

  /** Based upon amount of elapsed time in seconds. Eg. every 5 minutes, every hour, once a day, etc. */
  TIME = 1;

  /** Based upon number of transactions. Eg. every 100 transactions, every 5,000 transactions, etc. */
  TRANSACTION = 2;

  /** Based upon quantity of changed data in MB. Eg. every 100 MBs, every 5,000 MBs, etc. */
  DATA = 3;

  /** Based upon number of changed blobs. Eg. every 100 blobs, every 5,000 blobs, etc. */
  BLOB = 4;
}

/**
 * Subscription attributes and status
 */
struct SubscriptionDescriptor {
  /** ID of the subscription. MUST be unique within the global domain. */
  1: required i64 id;

  /** A string representing the subscription name. MUST be unique within the global domain/tenant. */
  2: required string name;

  /** The administrating tenant ID. */
  3: required i64 tenantID;

  /** ID of the local domain that is primary for this subscription. */
  4: required i32 primaryDomainID;

  /** ID of the volume that is the subject of this subscription. In particular, the ID of the volume in the primary local domain. */
  5: required i64 primaryVolumeID;

  /** ID of the replica domain for this subscription. */
  6: required i32 replicaDomainID;

  /** The date created, in epoch seconds. */
  7: required i64 createTime;

  /** The current state of the subscription. */
  8: required svc_types.ResourceState state;

  /** The type of subscription, generally whether it is content based or transaction based. */
  9: required SubscriptionType type;

  /** For content-based subscription types, the type of refresh scheduling policy. Also implies units. See the type definition. */
  10: required SubscriptionScheduleType scheduleType;

  /** When scheduling, the "size" of the interval the expiration of which results in a primary snapshot pushed to the replica. Units according to scheduleType. */
  11: required i64 intervalSize;
}

exception SubscriptionNotFound {
  1: string message;
}

exception SubscriptionNotModified {
  1: string message;
}

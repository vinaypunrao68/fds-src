/*
 * Copyright 2014 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "common.thrift"

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
  5: common.ResourceState state;
  /** the timeline time in seconds */
  6: i64  timelineTime;
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
struct LocalDomain {
  /** The Local Domain uuid */
  1: required i64 id;
  /** A string representing the name of the Local Domain, i.e. domain name */
  2: required string name;
  /** A string representing the location of the Local Domain. */
  3: required string site;
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
  /** a string reprenseting the user identifier of the user, i.e. user name */
  2: string identifier;
  /** a string representing the users password hash */
  3: string passwordHash;
  /** a string represetning the secret passphrase */
  4: string secret;
  /** a boolean flag indicating if the user has adminstration permissions */
  5: bool isFdsAdmin;
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
  /** the default access policy */
  6: optional common.VolumeAccessPolicy default_policy;
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
  6: common.ResourceState   state;
}

/*
 * Copyright 2014 by Formation Data Systems, Inc.
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
  SSD_ONLY    = 0;
  HDD_ONLY    = 1;
  HYBRID_ONLY = 3;
}

/**
 * Volume Storage Type
 */
enum VolumeType {
  OBJECT  = 0;
  BLOCK   = 1;
}

/* ------------------------------------------------------------
   Configuration Types
   ------------------------------------------------------------*/

/**
 * Snapshot and clone from CLI => OM structures 
 */
struct SnapshotPolicy {
  1: i64                    id;
  /** Name of the policy, should be unique */
  2: string                 policyName;
  /** Recurrence rule as per iCal format */
  3: string                 recurrenceRule;
  /** Retention time in seconds */
  4: i64                    retentionTimeSeconds;
  5: common.ResourceState   state;
  6: i64                    timelineTime;
}

/**
 * Statistic stream parameters.
 */
struct StreamingRegistrationMsg {
  1: i32            id;
  2: string         url;
  3: list<string>   volume_names;
  4: i32            sample_freq_seconds;
  5: i32            duration_seconds;
  6: string         http_method;
}

/**
 * Tenant descriptor.
 */
struct Tenant {
  1: i64    id;
  2: string identifier;
}

/**
 * A user of the system.
 */
struct User {
  1: i64    id;
  2: string identifier;
  3: string passwordHash;
  4: string secret;
  5: bool   isFdsAdmin;
}

/**
 * Volume configuration parameters.
 */
struct VolumeSettings {
  1: required i32               maxObjectSizeInBytes;
  2: required VolumeType        volumeType;
  3: required i64               blockDeviceSizeInBytes;
  4: required i64               contCommitlogRetention;
  5: MediaPolicy                mediaPolicy;
}

/**
 * Volume attributes and status
 */
struct VolumeDescriptor {
  1: required string            name;
  2: required i64               dateCreated;
  3: required VolumeSettings    policy;
  4: required i64               tenantId;
  5: i64                        volId;
  6: common.ResourceState   state;
}

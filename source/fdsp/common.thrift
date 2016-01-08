/*
 * Copyright 2014 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */
namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

const string INITIAL_XDI_VERSION = "0_8"
const string CURRENT_XDI_VERSION = INITIAL_XDI_VERSION

/**
 * By default, at the expiration of this period, fine-grained stats
 * will be streamed to registrations for them. It should be a multiple
 * of the Push&Aggregate constant used by Services to push what they've
 * collected to the aggregating DM who will then aggregate,
 * FdsStatPushAndAggregatePeriodSec.
 */
const i32 STAT_STREAM_FINE_GRAINED_FREQUENCY_SECONDS = 120
const i32 STAT_STREAM_RUN_FOR_EVER_DURATION = -1

enum BlobListOrder {
    UNSPECIFIED = 0,
    LEXICOGRAPHIC,
    BLOBSIZE
}

enum ErrorCode {
    OK = 0,
    INTERNAL_SERVER_ERROR,
    MISSING_RESOURCE,
    BAD_REQUEST,
    RESOURCE_ALREADY_EXISTS,
    RESOURCE_NOT_EMPTY,
    SERVICE_NOT_READY,
    SERVICE_SHUTTING_DOWN,
    TIMEOUT,
}

enum PatternSemantics {
    PCRE,
    PREFIX,
    PREFIX_AND_DELIMITER
}

exception ApiException {
    1: string message,
    2: ErrorCode errorCode,
}

exception NotMasterDomain {
    1: string message,
}

/* Can be consolidated when apis and fdsp merge or whatever */
struct BlobDescriptor {
     1: required string name,
     2: required i64 byteCount,
     3: required map<string, string> metadata
}

struct VolumeAccessMode {
  1: optional bool can_write = true;
  2: optional bool can_cache = true;
}


struct Initiator {
    /** world-wide name */
    1: required string wwn_mask
}

/** iSCSI */
struct LogicalUnitNumber {
    /** logical unit name */
    1: required string name,
    /** access mode */
    2: required string access
}

struct Credentials {
    /** the user name */
    1: required string name,
    /** the user's password */
    2: required string passwd
}

struct IScsiTarget {
    /** a unordered list of logical unit numbers */
    1: required list<LogicalUnitNumber> luns,
    /** a unordered list of incoming user credentials */
    2: optional list<Credentials> incomingUsers,
    /** a unordered list of outgoing user credentials */
    3: optional list<Credentials> outgoingUsers,
    /** initiators */
    4: optional list<Initiator> initiators;
}

/** NFS Options Map */
struct NfsOption {
    /** nfs option */
    1: optional string options
    /** ip filters */
    2: optional string client
}

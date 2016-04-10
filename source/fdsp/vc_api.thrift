/*
 * Copyright 2014-2016 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "common.thrift"
include "svc_api.thrift"
include "svc_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.dm

/**
 * A message sent to a DM to mark the volume offline and to start the checking process
 */
struct CheckVolumeMetaDataMsg {
    1: required i64                 volume_id;
    2: required svc_types.SvcUuid   volCheckerNodeUuid;
    /* The max number of levelDB entires to hash every time */
    3: i32                          batch_size;
}

/**
 * Empty response
 */
struct CheckVolumeMetaDataRspMsg {
    1: required binary              hash_result;
}

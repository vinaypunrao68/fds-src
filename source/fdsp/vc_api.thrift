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
    1: required i64         volumeId;
}

/**
 * Empty response
 */
struct CheckVolumeMetaDataMsgRsp {
}

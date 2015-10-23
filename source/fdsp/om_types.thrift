/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"
include "pm_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om.types
namespace java com.formationds.protocol.pm.types
namespace java com.formationds.protocol

exception OmRegisterException {
}

exception SvcLookupException {
     1: string message;
}

struct SvcChangeReqInfo {
  /* Type of the service for which state change req came in */
  1: required common.FDSP_MgrIdType svcType;
  /* This value(0/1) will indicate (no-op/op success) */
  2: required pm_types.ActionCode actionCode;
}

typedef list<SvcChangeReqInfo> SvcChangeInfoList

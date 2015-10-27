/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol
namespace java com.formationds.protocol.om.types
namespace java com.formationds.protocol.pm.types

include "common.thrift"
include "pm_types.thrift"
include "svc_types.thrift"

exception OmRegisterException {
}

exception SvcLookupException {
     1: string message;
}

struct SvcChangeReqInfo {
  /* Type of the service for which state change req came in */
  1: svc_types.FDSP_MgrIdType svcType;
  /* This value(0/1) will indicate (no-op/op success) */
  2: pm_types.ActionCode actionCode;
}

typedef list<SvcChangeReqInfo> SvcChangeInfoList

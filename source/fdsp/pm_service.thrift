/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

include "svc_types.thrift"
include "svc_api.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.pm

/*
 * --------------------------------------------------------------------------------
 * Service registration/deregestration handshake
 * --------------------------------------------------------------------------------
 */

/**
 * Deactivate Services message to PM 
 * @deprecated 06/09/2015
 */
struct DeactivateServicesMsg {
  1: svc_types.FDSP_Uuid  node_uuid,
  2: string     node_name,          /* autogenerated name */
  3: bool       deactivate_sm_svc,     /* true if deactivate sm service */
  4: bool       deactivate_dm_svc,     /* true if deactivate dm service */
  5: bool       deactivate_om_svc,     /* true if deactivate om service */
  6: bool       deactivate_am_svc      /* true if deactivate am service */
}

/**
 * Deactivate Services Response message from PM
 */
struct DeactivateServicesMsgRsp {
}

/**
 * Notify disk-map change message from PM
 */
struct NotifyDiskMapChange {
}


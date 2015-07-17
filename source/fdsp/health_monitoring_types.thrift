/*
 * Copyright 2015 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "common.thrift"
include "svc_types.thrift"

/**
 * @author    Donavan Nelson <donavan @ formationds.com>
 * @version   0.7
 * @since     2015-05-30
 */

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om.types

enum HealthState {
  HEALTH_STATE_RUNNING             = 404000;
  HEALTH_STATE_INITIALIZING        = 404001;
  HEALTH_STATE_DEGRADED            = 404002;
  HEALTH_STATE_LIMITED             = 404003;
  HEALTH_STATE_SHUTTING_DOWN       = 404004;
  HEALTH_STATE_ERROR               = 404005;
  HEALTH_STATE_UNREACHABLE         = 404006;
  HEALTH_STATE_UNEXPECTED_EXIT     = 404007;
}

/**
 * Health Information Message
 */
struct HealthInfoMessage {
  1: required svc_types.SvcInfo         serviceInfo;
  2: required HealthState               serviceState;
  3: common.SvcID                       platformUUID;     // Only intended to be used when Platformd spoofs a HealthInfoMessage
  4: i32                                statusCode;       // This should be a value in fds_errno_t
  5: string                             statusInfo;
}

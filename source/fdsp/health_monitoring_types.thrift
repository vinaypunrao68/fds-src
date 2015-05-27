/*
 * Copyright 2015 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "common.thrift"

namespace cpp fds.apis
namespace java com.formationds.protocol.om.types

enum HealthState {
  RUNNING          = 404000;
  INITIALIZING     = 404001;
  DEGRADED         = 404002;
  LIMITED          = 404003;
  SHUTTING_DOWN    = 404004;
  ERROR            = 404005;
  UNREACHABLE      = 404006;
  UNEXPECTED_EXIT  = 404007;
}

/**
 * Health Information Message
 */
struct HealthInfoMessage {
  1: required common.SvcID  serviceID;
  2: required i32           servicePort;
  3: required HealthState   serviceState;
  4: i32                    statusCode;       // This should be a value in fds_errno_t
  5: string                 statusInfo;
}


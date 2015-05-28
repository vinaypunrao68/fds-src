/*
 * Copyright 2015 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "health_monitoring_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om

struct NotifyHealthReport {
    1: health_monitoring_types.HealthInfoMessage   healthReport;
}

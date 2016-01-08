/*
 * Copyright 2015 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "health_monitoring_types.thrift"
include "svc_types.thrift"

/*
 * @author    Donavan Nelson <donavan @ formationds.com>
 * @version   0.7
 * @since     2015-05-30
 */

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om
namespace java com.formationds.protocol.pm

/**
 * Send a health report notification to the OM
 * <p>
 * This function is used by all services to send periodic health report notifications to the OM.
 * Should a service die unexpectedly, the PM will send a health report notification to the OM
 * on behalf of the process that died.
 *
 * @param   healthReport  A populated healthInforMessage
 * @return  Nothing
 */
struct NotifyHealthReport {
    1: health_monitoring_types.HealthInfoMessage   healthReport;
}

/**
 * Structure to check whether PM is
 * alive
 */
struct HeartbeatMessage {
    1: svc_types.FDSP_Uuid svcUuid,
    2: double timestamp;
}

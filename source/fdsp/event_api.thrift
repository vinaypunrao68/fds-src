
/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

include "event_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om.event

struct NotifyEventMsg {
 1: event_types.Event event;
}

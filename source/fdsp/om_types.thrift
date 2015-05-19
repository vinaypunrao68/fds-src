/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om.types

exception OmRegisterException {
}

exception SvcLookupException {
     1: string message;
}

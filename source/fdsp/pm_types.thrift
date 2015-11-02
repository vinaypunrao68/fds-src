/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.pm.types

enum pmServiceStateTypeId
{
    SERVICE_NOT_PRESENT = 0x6000;
    SERVICE_NOT_RUNNING = 0x6001;
    SERVICE_RUNNING     = 0x6002;
}

enum ActionCode
{
    NO_ACTION = 0x0;
    STARTED   = 0x1;
}

/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "FDSP.thrift"
include "fds_service.thrift"

namespace cpp FDS_ProtocolInterface

struct CtrlCreateBucket {
     1: FDSP.FDSP_CreateVolType       cv;
}

struct CtrlDeleteBucket {
    1:  FDSP.FDSP_DeleteVolType       dv;
}

struct CtrlModifyBucket {
    1:  FDSP.FDSP_ModifyVolType      mv;
}

struct CtrlSvcEvent {
    1: required fds_service.SvcUuid    evt_src_svc_uuid; // The svc uuid that this event targets
    2: required i32        evt_code;         // The error itself
    3: fds_service.FDSPMsgTypeId       evt_msg_type_id;  // The msg that trigged this event (if any)
}

struct CtrlTestBucket {
     1: FDSP.FDSP_TestBucket           tbmsg;
}

struct CtrlTokenMigrationAbort {
}

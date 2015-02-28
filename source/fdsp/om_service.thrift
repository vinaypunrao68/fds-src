/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"
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
    1: required common.SvcUuid    evt_src_svc_uuid; // The svc uuid that this event targets
    2: required i32        evt_code;         // The error itself
    3: fds_service.FDSPMsgTypeId       evt_msg_type_id;  // The msg that trigged this event (if any)
}

struct CtrlTestBucket {
     1: FDSP.FDSP_TestBucket           tbmsg;
}

struct CtrlTokenMigrationAbort {
}

exception OmRegisterException {
}

/**
 * OM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service OMSvc extends fds_service.PlatNetSvc {
    void registerService(1: fds_service.SvcInfo svcInfo) throws (1: OmRegisterException e);
    list<fds_service.SvcInfo> getSvcMap(1: i32 nullarg);
}

package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_OMControlPathReq;
import FDS_ProtocolInterface.FDSP_OMControlPathResp;

public class FakeOm {
    public static void main(String[] args) {
        FdspProxy<FDSP_OMControlPathReq, FDSP_OMControlPathResp> proxy = new FdspProxy<>(FDSP_OMControlPathReq.class, FDSP_OMControlPathResp.class);
    }
}

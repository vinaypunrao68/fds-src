package com.formationds;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_AnnounceDiskCapability;
import FDS_ProtocolInterface.FDSP_RegisterNodeType;
import com.formationds.protocol.svc.types.FDSP_MgrIdType;
import com.formationds.protocol.svc.types.FDSP_Uuid;

import java.util.UUID;

public abstract class BaseTestCase {
    protected FDSP_RegisterNodeType makeEndpoint(String name, FDSP_MgrIdType nodeType) {
        FDSP_RegisterNodeType node = new FDSP_RegisterNodeType(nodeType,
                name,
                0,
                0,
                0,
                100,
                101,
                102,
                new FDSP_Uuid(UUID.randomUUID().getMostSignificantBits()),
                new FDSP_Uuid(UUID.randomUUID().getMostSignificantBits()),
                new FDSP_AnnounceDiskCapability(),
                null,
                0);
        return node;
    }
}

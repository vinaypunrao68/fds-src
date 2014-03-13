package com.formationds;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_AnnounceDiskCapability;
import FDS_ProtocolInterface.FDSP_MgrIdType;
import FDS_ProtocolInterface.FDSP_RegisterNodeType;
import FDS_ProtocolInterface.FDSP_Uuid;
import junit.framework.TestCase;

import java.util.UUID;

public abstract class BaseTestCase extends TestCase {
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
                new FDSP_AnnounceDiskCapability());
        return node;
    }
}

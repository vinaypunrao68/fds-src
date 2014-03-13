package com.formationds.spike.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_MgrIdType;
import com.formationds.BaseTestCase;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class ServiceDirectoryTest extends BaseTestCase {
    @Test
    public void testTraverse() {
        ServiceDirectory serviceDirectory = new ServiceDirectory();
        serviceDirectory.add(makeEndpoint("foo", FDSP_MgrIdType.FDSP_STOR_HVISOR));
        serviceDirectory.add(makeEndpoint("bar", FDSP_MgrIdType.FDSP_STOR_HVISOR));
        serviceDirectory.add(makeEndpoint("panda", FDSP_MgrIdType.FDSP_STOR_MGR));
        serviceDirectory.add(makeEndpoint("hello", FDSP_MgrIdType.FDSP_DATA_MGR));
        assertEquals(4, serviceDirectory.allNodes().count());
        assertEquals(2, serviceDirectory.byType(FDSP_MgrIdType.FDSP_STOR_HVISOR).count());
        assertEquals(1, serviceDirectory.byType(FDSP_MgrIdType.FDSP_STOR_MGR).count());
        assertEquals(1, serviceDirectory.byType(FDSP_MgrIdType.FDSP_DATA_MGR).count());
    }
}

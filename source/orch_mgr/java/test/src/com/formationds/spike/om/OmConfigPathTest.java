package com.formationds.spike.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.BaseTestCase;
import com.formationds.spike.ServiceDirectory;
import org.junit.Test;

import java.util.Arrays;

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.*;

public class OmConfigPathTest extends BaseTestCase {
    @Test
    public void testActivateAllNodes() throws Exception {
        ServiceDirectory serviceDirectory = mock(ServiceDirectory.class);
        FDSP_ControlPathReq.Iface mockClient = mock(FDSP_ControlPathReq.Iface.class);
        OmConfigPath omConfigPath = new OmConfigPath(serviceDirectory, node -> mockClient);
        FDSP_RegisterNodeType[] nodes = {
                makeEndpoint("foo", FDSP_MgrIdType.FDSP_STOR_MGR),
                makeEndpoint("bar", FDSP_MgrIdType.FDSP_DATA_MGR),
                makeEndpoint("bar", FDSP_MgrIdType.FDSP_PLATFORM)
        };
        when(serviceDirectory.allNodes()).thenReturn(Arrays.stream(nodes));
        omConfigPath.ActivateAllNodes(new FDSP_MsgHdrType(), new FDSP_ActivateAllNodesType());
        verify(mockClient, times(1)).NotifyNodeActive(any(FDSP_MsgHdrType.class), any(FDSP_ActivateNodeType.class));
    }
}

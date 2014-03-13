package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_Uuid;
import junit.framework.TestCase;

import java.util.Collection;
import java.util.List;

public class DltTest extends TestCase {
    public void testDeserialize() throws Exception {
        Dlt dlt = new DltSerializer().deserialize(frame);
        assertEquals(1, dlt.getVersion());
        assertEquals(8, dlt.getNumBitsForToken());
        assertEquals(1, dlt.getDepth());
        assertEquals(256, dlt.getNumTokens());
        List<FDSP_Uuid> uuids = dlt.getUuids();
        assertEquals(1, uuids.size());
        long uuid = 1680041476237363857l;
        assertEquals(uuid, uuids.get(0).getUuid());
        for (int i = 0; i < dlt.getNumTokens(); i++) {
            Collection<FDSP_Uuid> tokenPlacement = dlt.getTokenPlacement(0);
            assertEquals(1, tokenPlacement.size());
            assertEquals(uuid, tokenPlacement.iterator().next().getUuid());
        }
    }


    byte[] frame = new byte[] {0,0,0,0,0,0,0,1,0,0,0,8,0,0,0,1,0,0,1,0,0,0,0,1,23,80,-76,-46,71,13,10,-111,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

}

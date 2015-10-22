package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.svc.types.FDSP_Uuid;
import com.google.common.collect.Lists;
import org.junit.Test;

import java.util.Collection;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class DltTest {
    long uuid = 1680041476237363857l;

    @Test
    public void testDeserialize() throws Exception {
        Dlt dlt = new Dlt(frame);
        assertEquals(1, dlt.getVersion());
        assertEquals(8, dlt.getNumBitsForToken());
        assertEquals(1, dlt.getDepth());
        assertEquals(256, dlt.getNumTokens());
        for (int i = 0; i < dlt.getNumTokens(); i++) {
            Collection<FDSP_Uuid> tokenPlacement = dlt.getTokenPlacement(0);
            assertEquals(1, tokenPlacement.size());
            assertEquals(uuid, tokenPlacement.iterator().next().getUuid());
        }
    }

    @Test
    public void testSerialize() throws Exception {
        Dlt dlt = new Dlt(1, 8, 1, 256, new FDSP_Uuid[] {new FDSP_Uuid(uuid)});
        for (int i = 0; i < dlt.getNumTokens(); i++) {
            dlt.placeToken(i, 0, 0);
        }
        byte[] frozen = dlt.serialize();
        assertArrayEquals(frame, frozen);
    }

    byte[] frame = new byte[] {0,0,0,0,0,0,0,1,0,0,0,8,0,0,0,1,0,0,1,0,0,0,0,1,23,80,-76,-46,71,13,10,-111,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

}

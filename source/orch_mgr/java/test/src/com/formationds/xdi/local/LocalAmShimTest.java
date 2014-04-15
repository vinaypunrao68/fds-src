package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.VolumeDescriptor;
import org.junit.Test;

import java.nio.ByteBuffer;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class LocalAmShimTest {

    @Test
    public void testVolumes() throws Exception {
        LocalAmShim shim = new LocalAmShim();
        shim.createDomain("foo");
        shim.createVolume("foo", "v1", new VolumeDescriptor(8));
        shim.createVolume("foo", "v2", new VolumeDescriptor(32));
        assertEquals(2, shim.listVolumes("foo").size());
        shim.deleteVolume("foo", "v2");
        assertEquals(1, shim.listVolumes("foo").size());
        assertEquals(8, shim.statVolume("foo", "v1").getObjectSizeInBytes());

        shim.updateBlob("foo", "v1", "blob", ByteBuffer.wrap(new byte[] {1, 2, 3, 4, 5}), 4, 5);
        assertArrayEquals(new byte[] {0, 0, 0, 0, 0, 1, 2, 3}, shim.getBlob("foo", "v1", "blob", 8, 0).array());
        assertArrayEquals(new byte[] {4, 0, 0, 0, 0, 0, 0, 0}, shim.getBlob("foo", "v1", "blob", 8, 8).array());
    }
}

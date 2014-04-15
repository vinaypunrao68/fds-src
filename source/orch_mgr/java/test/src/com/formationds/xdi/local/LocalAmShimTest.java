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
        String domainName = "foo";
        String volumeName = "v1";
        String blobName = "blob";

        shim.createDomain(domainName);
        shim.createVolume(domainName, volumeName, new VolumeDescriptor(8));
        shim.createVolume(domainName, "v2", new VolumeDescriptor(32));
        assertEquals(2, shim.listVolumes(domainName).size());
        shim.deleteVolume(domainName, "v2");
        assertEquals(1, shim.listVolumes(domainName).size());
        assertEquals(8, shim.statVolume(domainName, volumeName).getObjectSizeInBytes());

        ByteBuffer buffer = ByteBuffer.wrap(new byte[]{1, 2, 3, 4, 5});
        shim.updateBlob(domainName, volumeName, blobName, buffer, 4, 5);
        Blob blob = shim.getBlob(domainName, volumeName, blobName);
        assertEquals(2, blob.getBlocks().size());

        assertArrayEquals(new byte[]{0, 0, 0, 0, 0, 1, 2, 3}, shim.getBlob(domainName, volumeName, blobName, 8, 0).array());
        assertArrayEquals(new byte[] {4, 0, 0, 0, 0, 0, 0, 0}, shim.getBlob(domainName, volumeName, blobName, 8, 8).array());
    }
}

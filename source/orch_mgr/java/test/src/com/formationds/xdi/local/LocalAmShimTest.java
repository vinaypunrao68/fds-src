package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.BlobDescriptor;
import com.formationds.xdi.shim.Uuid;
import com.formationds.xdi.shim.VolumePolicy;
import com.google.common.base.Joiner;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

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
        shim.createVolume(domainName, volumeName, new VolumePolicy(8));
        shim.createVolume(domainName, "v2", new VolumePolicy(32));

        assertEquals(2, shim.listVolumes(domainName).size());
        shim.deleteVolume(domainName, "v2");
        assertEquals(1, shim.listVolumes(domainName).size());
        assertEquals(8, shim.statVolume(domainName, volumeName).getPolicy().getObjectSizeInBytes());

        ByteBuffer buffer = ByteBuffer.wrap(new byte[]{1, 2, 3, 4, 5});
        shim.updateBlob(domainName, volumeName, blobName, new Uuid(), buffer, 4, 5);
        Blob blob = shim.getBlob(domainName, volumeName, blobName);
        assertEquals(2, blob.getBlocks().size());

        assertArrayEquals(new byte[]{0, 0, 0, 0, 0, 1, 2, 3}, shim.getBlob(domainName, volumeName, blobName, 8, 0).array());
        assertArrayEquals(new byte[] {4, 0, 0, 0, 0, 0, 0, 0}, shim.getBlob(domainName, volumeName, blobName, 8, 8).array());

        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        metadata.put("goodnight", "moon");
        shim.updateMetadata(domainName, volumeName, blobName, new Uuid(), metadata);
        BlobDescriptor descriptor = shim.statBlob(domainName, volumeName, blobName);
        Map<String, String> m = descriptor.getMetadata();
        assertEquals(2, m.keySet().size());
        assertEquals("world", m.get("hello"));
        assertEquals("moon", m.get("goodnight"));
        assertEquals(9, descriptor.getByteCount());

        shim.updateBlob(domainName, volumeName, "otherBlob", new Uuid(), buffer, 4, 5);
        assertEquals("blob, otherBlob", Joiner.on(", ").join(shim.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0)));

        assertEquals("otherBlob", Joiner.on(", ").join(shim.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 1)));

        shim.deleteBlob(domainName, volumeName, "otherBlob");
        assertEquals("blob", Joiner.on(", ").join(shim.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0)));
    }
}

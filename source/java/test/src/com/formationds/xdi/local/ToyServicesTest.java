package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.BlobDescriptor;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.apis.TxDescriptor;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class ToyServicesTest {

    @Test
    public void testVolumes() throws Exception {
        ToyServices shim = new ToyServices("local");
        String domainName = "foo";
        String volumeName = "v1";
        String blobName = "blob";

        shim.createDomain(domainName);
        shim.createVolume(domainName, volumeName,
                          new VolumeSettings(8, VolumeType.OBJECT, 0));
        shim.createVolume(domainName, "v2",
                          new VolumeSettings(32, VolumeType.OBJECT, 0));

        assertEquals(2, shim.listVolumes(domainName).size());
        shim.deleteVolume(domainName, "v2");
        assertEquals(1, shim.listVolumes(domainName).size());
        assertEquals(8, shim.statVolume(domainName, volumeName).getPolicy().getMaxObjectSizeInBytes());

        TxDescriptor txDesc = shim.startBlobTx(domainName, volumeName, blobName);
        ByteBuffer buffer = ByteBuffer.wrap(new byte[]{1, 2, 3, 4, 5});
        shim.updateBlob(domainName, volumeName, blobName, txDesc,
                        buffer, 4, new ObjectOffset(0),
                        ByteBuffer.wrap(new byte[0]), false);
        shim.updateBlob(domainName, volumeName, blobName, txDesc,
                        buffer, 5, new ObjectOffset(1),
                        ByteBuffer.wrap(new byte[] {42, 43}), true);

        BlobDescriptor blob = shim.statBlob(domainName, volumeName, blobName);
        assertEquals(13, blob.getByteCount());
        assertArrayEquals(new byte[] {42, 43}, blob.getDigest());

        assertArrayEquals(new byte[]{1, 2, 3, 4, 0, 0, 0, 0}, shim.getBlob(domainName, volumeName, blobName, 8, new ObjectOffset(0)).array());
        assertArrayEquals(new byte[] {1, 2, 3, 4, 5, 0, 0, 0}, shim.getBlob(domainName, volumeName, blobName, 8, new ObjectOffset(1)).array());

        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        metadata.put("goodnight", "moon");
        shim.updateMetadata(domainName, volumeName, blobName, txDesc, metadata);
        BlobDescriptor descriptor = shim.statBlob(domainName, volumeName, blobName);
        Map<String, String> m = descriptor.getMetadata();
        assertEquals(2, m.keySet().size());
        assertEquals("world", m.get("hello"));
        assertEquals("moon", m.get("goodnight"));
        assertEquals(13, descriptor.getByteCount());

        String otherBlob = "otherBlob";
        shim.updateBlob(domainName, volumeName, otherBlob, txDesc,
                        buffer, 4, new ObjectOffset(0),
                        ByteBuffer.wrap(new byte[0]), true);
        List<BlobDescriptor> parts = shim.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0);
        assertEquals(2, parts.size());

        parts = shim.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 1);
        assertEquals(1, parts.size());
        assertEquals(otherBlob, parts.get(0).getName());

        shim.deleteBlob(domainName, volumeName, otherBlob);
        parts = shim.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0);
        assertEquals(1, parts.size());
        assertEquals("blob", parts.get(0).getName());
    }
}

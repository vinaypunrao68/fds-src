package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.*;
import com.google.common.collect.Lists;
import com.google.common.collect.Maps;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.List;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class BlockIteratorTest {
    @Test
    public void testIterator() throws Exception {
        AmShim.Iface am = mock(AmShim.Iface.class);
        String domainName = "domain";
        String volumeName = "volume";
        String blobName = "blob";
        int blockSize = 4;
        ByteBuffer[] blocks = new ByteBuffer[] {
                ByteBuffer.wrap(new byte[] {0, 1, 2, 3}),
                ByteBuffer.wrap(new byte[] {4, 5, 6}),
        };

        VolumeDescriptor volumeDescriptor = new VolumeDescriptor("foo", 0, new VolumePolicy(blockSize));
        BlobDescriptor blobDescriptor = new BlobDescriptor(blobName, 7, ByteBuffer.wrap(new byte[0]), Maps.newHashMap());

        when(am.statVolume(domainName, volumeName)).thenReturn(volumeDescriptor);
        when(am.statBlob(domainName, volumeName, blobName)).thenReturn(blobDescriptor);

        when(am.getBlob(domainName, volumeName, blobName, 4, new ObjectOffset(0))).thenReturn(blocks[0]);
        when(am.getBlob(domainName, volumeName, blobName, 3, new ObjectOffset(1))).thenReturn(blocks[1]);

        Iterator<byte[]> result = new BlockIterator(am).read(domainName, volumeName, blobName);
        List<byte[]> list = Lists.newArrayList(result);
        assertEquals(2, list.size());
        assertArrayEquals(blocks[0].array(), list.get(0));
        assertArrayEquals(blocks[1].array(), list.get(1));
    }
}

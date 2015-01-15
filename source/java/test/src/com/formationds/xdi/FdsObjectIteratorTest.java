package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.thrift.OMConfigServiceClient;
import com.google.common.collect.Maps;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.Iterator;

import static org.junit.Assert.*;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import FDS_ProtocolInterface.ResourceState;
public class FdsObjectIteratorTest {
    @Test
    public void testIterator() throws Exception {
        AuthenticationToken token = mock(AuthenticationToken.class);
        AmService.Iface am = mock(AmService.Iface.class);
        OMConfigServiceClient config = mock(OMConfigServiceClient.class);
        String domainName = "domain";
        String volumeName = "volume";
        String blobName = "blob";
        int blockSize = 4;
        ByteBuffer[] blocks = new ByteBuffer[] {
                ByteBuffer.wrap(new byte[] {0, 1, 2, 3}),
                ByteBuffer.wrap(new byte[] {4, 5, 6}),
        };

        VolumeDescriptor volumeDescriptor = new VolumeDescriptor("foo", 0, new VolumeSettings(blockSize, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY ), 0, 0, ResourceState.Unknown);

        BlobDescriptor blobDescriptor = new BlobDescriptor(blobName, 7, Maps.newHashMap());

        when(token.signature(any())).thenReturn("mocked-token");
        when(config.statVolume(token, domainName, volumeName)).thenReturn(volumeDescriptor);
        when(am.statBlob(domainName, volumeName, blobName)).thenReturn(blobDescriptor);

        when(am.getBlob(domainName, volumeName, blobName, 4, new ObjectOffset(0))).thenAnswer(o -> blocks[0].slice());
        when(am.getBlob(domainName, volumeName, blobName, 3, new ObjectOffset(1))).thenAnswer(o -> blocks[1].slice());
        when(am.getBlob(domainName, volumeName, blobName, 2, new ObjectOffset(1))).thenAnswer(o -> ByteBuffer.wrap(new byte[] {4, 5}));

        FdsObjectIterator iterator = new FdsObjectIterator(am, config);

        Iterator<byte[]> result = iterator.read(token, domainName, volumeName, blobName);
        assertTrue(result.hasNext());
        byte[] next = result.next();
        assertArrayEquals(blocks[0].array(), next);

        assertTrue(result.hasNext());
        next = result.next();
        assertArrayEquals(blocks[1].array(), next);
        assertFalse(result.hasNext());


        result = iterator.read(token, domainName, volumeName, blobName, 2, 4);
        assertTrue(result.hasNext());
        next = result.next();
        assertArrayEquals(new byte[] {2, 3}, next);

        assertTrue(result.hasNext());
        next = result.next();
        assertArrayEquals(new byte[] {4, 5}, next);
        assertFalse(result.hasNext());
    }
}

package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ObjectOffset;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

import static org.mockito.Mockito.*;

public class StreamWriterTest {
    @Test
    public void testWriteStream() throws Exception {
        InputStream in = new ByteArrayInputStream(new byte[]{0, 1, 2, 3, 4, 5, 6, 7});
        AmService.Iface mockAm = mock(AmService.Iface.class);
        String domainName = "domain";
        String volumeName = "volume";
        String blobName = "blob";
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");

        new StreamWriter(4, mockAm).write(domainName, volumeName, blobName, in, metadata);

        verify(mockAm, times(1)).updateBlob(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                any(ByteBuffer.class),
                eq(4),
                eq(new ObjectOffset(0)),
                eq(ByteBuffer.wrap(new byte[0])),
                eq(false));

        verify(mockAm, times(1)).updateBlob(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                any(ByteBuffer.class),
                eq(4),
                eq(new ObjectOffset(1)),
                eq(ByteBuffer.wrap(new byte[0])),
                eq(false));

        byte[] expectedDigest = new byte[] {54,119,80,-105,81,-52,-10,21,57,23,77,43,-106,53,-89,-65};

        verify(mockAm, times(1)).updateBlob(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                any(ByteBuffer.class),
                eq(4),
                eq(new ObjectOffset(1)),
                eq(ByteBuffer.wrap(expectedDigest)),
                eq(true));

        verify(mockAm, times(1)).updateMetadata(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                eq(metadata)
        );
    }
}

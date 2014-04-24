package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.ObjectOffset;
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
        AmShim.Iface mockAm = mock(AmShim.Iface.class);
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
                any(ByteBuffer.class),
                eq(false));

        verify(mockAm, times(1)).updateBlob(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                any(ByteBuffer.class),
                eq(4),
                eq(new ObjectOffset(1)),
                any(ByteBuffer.class),
                eq(false));

        verify(mockAm, times(1)).updateBlob(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                any(ByteBuffer.class),
                eq(4),
                eq(new ObjectOffset(1)),
                any(ByteBuffer.class),
                eq(true));

        verify(mockAm, times(1)).updateMetadata(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                eq(metadata)
        );
    }
}

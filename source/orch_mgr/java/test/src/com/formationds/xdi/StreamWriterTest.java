package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.Uuid;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.*;

public class StreamWriterTest {
    @Test
    public void testWriteStream() throws Exception {
        assertEquals(ByteBuffer.wrap(new byte[]{1, 2}), ByteBuffer.wrap(new byte[]{1, 2, 3}, 0, 2));

        InputStream in = new ByteArrayInputStream(new byte[]{0, 1, 2, 3, 4, 5, 6, 7});
        AmShim.Iface mockAm = mock(AmShim.Iface.class);
        String domainName = "domain";
        String volumeName = "volume";
        String blobName = "blob";
        Uuid txId = new Uuid(17, 42);
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");

        when(mockAm.startBlobTx(domainName, volumeName, blobName)).thenReturn(txId);

        new StreamWriter(4, mockAm).write(domainName, volumeName, blobName, in, metadata);

        verify(mockAm, times(1)).startBlobTx(domainName, volumeName, blobName);

        verify(mockAm, times(1)).updateBlob(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                eq(txId),
                any(ByteBuffer.class),
                eq(4),
                eq(0l));

        verify(mockAm, times(1)).updateBlob(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                eq(txId),
                any(ByteBuffer.class),
                eq(4),
                eq(4l));

        verify(mockAm, times(1)).updateMetadata(
                eq(domainName),
                eq(volumeName),
                eq(blobName),
                eq(txId),
                eq(metadata)
        );

        verify(mockAm, times(1)).commit(txId);
    }
}

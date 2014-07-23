package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.util.ChunkingInputStream;
import org.apache.thrift.TException;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.junit.Assert.*;


public class StreamWriterTest {

    @Test
    public void testWriteStream() throws Exception {
        ByteArrayInputStream bytes = new ByteArrayInputStream(new byte[]{0, 1, 2, 3, 4, 5, 6, 7});
        InputStream in = new ChunkingInputStream(bytes, 2);

        String domainName = "domain";
        String volumeName = "volume";
        String blobName = "blob";
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        StubAm am = new StubAm();

        byte[] result = new StreamWriter(4, am).write(domainName, volumeName, blobName, in, metadata);
        assertEquals(3, am.objectsWritten.size());
        assertArrayEquals(new byte[]{0, 1, 2, 3}, am.objectsWritten.get(0));
        assertArrayEquals(new byte[]{4, 5, 6, 7}, am.objectsWritten.get(1));
        assertArrayEquals(new byte[]{4, 5, 6, 7}, am.objectsWritten.get(2));
        assertArrayEquals(new byte[]{54, 119, 80, -105, 81, -52, -10, 21, 57, 23, 77, 43, -106, 53, -89, -65}, result);
        assertTrue(am.isCommitted);
    }

    private class StubAm implements AmService.Iface {
        boolean isCommitted = false;
        List<byte[]> objectsWritten = new ArrayList<>();

        @Override
        public void attachVolume(String domainName, String volumeName) throws ApiException, TException {

        }

        @Override
        public List<BlobDescriptor> volumeContents(String domainName, String volumeName, int count, long offset) throws ApiException, TException {
            return null;
        }

        @Override
        public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
            return null;
        }

        @Override
        public TxDescriptor startBlobTx(String domainName, String volumeName, String blobName, int blobMode) throws ApiException, TException {
            return null;
        }

        @Override
        public void commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDesc) throws ApiException, TException {
        }

        @Override
        public void abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDesc) throws ApiException, TException {
        }

        @Override
        public ByteBuffer getBlob(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) throws ApiException, TException {
            return null;
        }

        @Override
        public void updateMetadata(String domainName, String volumeName, String blobName, TxDescriptor txDesc, Map<String, String> metadata) throws ApiException, TException {

        }

        @Override
        public void updateBlob(String domainName, String volumeName, String blobName, TxDescriptor txDesc, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) throws ApiException, TException {
            byte[] buf = new byte[length];
            System.arraycopy(bytes.array(), 0, buf, 0, length);
            objectsWritten.add(buf);
        }

        @Override
        public void deleteBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {

        }

        @Override
        public VolumeStatus volumeStatus(String domainName, String volumeName) throws ApiException, TException {
            return null;
        }
    }
}

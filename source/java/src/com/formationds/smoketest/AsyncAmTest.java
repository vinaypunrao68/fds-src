package com.formationds.smoketest;

import com.formationds.apis.*;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.ErrorCode;
import com.formationds.util.ByteBufferUtility;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import com.google.common.collect.Maps;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;


@Ignore
public class AsyncAmTest extends BaseAmTest {

    @Test
    public void testVolumeContents() throws Exception {
        List<BlobDescriptor> contents = asyncAm.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0).get();
        assertEquals(0, contents.size());
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(0), metadata).get();
        contents = asyncAm.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0).get();
        assertEquals(1, contents.size());
    }

    @Test
    public void testDeleteBlob() throws Exception {
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(0), metadata).get();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals("world", blobDescriptor.getMetadata().get("hello"));
        asyncAm.deleteBlob(domainName, volumeName, blobName).get();
        try {
            asyncAm.statBlob(domainName, volumeName, blobName).get();
            fail("Should have gotten an ExecutionException");
        } catch (ExecutionException e) {
            ApiException apiException = (ApiException) e.getCause();
            assertEquals(ErrorCode.MISSING_RESOURCE, apiException.getErrorCode());
        }
    }

    @Test
    public void testStatVolume() throws Exception {
        VolumeStatus volumeStatus = asyncAm.volumeStatus(domainName, volumeName).get();
        assertEquals(0, volumeStatus.blobCount);
    }

    @Test
    public void testUpdateMetadata() throws Exception {
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(0), metadata).get();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals("world", blobDescriptor.getMetadata().get("hello"));

        metadata.put("hello", "panda");
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(0), metadata).get();
        blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals("panda", blobDescriptor.getMetadata().get("hello"));
    }

    @Test
    public void testTransactions() throws Exception {
        asyncAm.startBlobTx(domainName, volumeName, blobName, 1)
                .thenCompose(tx -> asyncAm.updateBlob(domainName, volumeName, blobName, tx, bigObject, OBJECT_SIZE, new ObjectOffset(0), false).thenApply(x -> tx))
                .thenCompose(tx -> asyncAm.updateBlob(domainName, volumeName, blobName, tx, smallObject, smallObjectLength, new ObjectOffset(1), true).thenApply(x -> tx))
                .thenCompose(tx -> asyncAm.commitBlobTx(domainName, volumeName, blobName, tx))
                .thenCompose(x -> asyncAm.statBlob(domainName, volumeName, blobName))
                .thenAccept(bd -> assertEquals(OBJECT_SIZE + smallObjectLength, bd.getByteCount()))
                .get();
    }

    @Test
    public void testTransactionalMetadataUpdate() throws Exception {
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(0), metadata).get();
        TxDescriptor tx = asyncAm.startBlobTx(domainName, volumeName, blobName, 1).get();
        metadata.put("animal", "panda");
        asyncAm.updateMetadata(domainName, volumeName, blobName, tx, metadata).get();
        asyncAm.commitBlobTx(domainName, volumeName, blobName, tx).get();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals("panda", blobDescriptor.getMetadata().get("animal"));
        assertEquals(smallObjectLength, blobDescriptor.getByteCount());
    }

    @Test
    public void testMultipleAsyncUpdates() throws Exception {
        String blobName = UUID.randomUUID().toString();
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, bigObject, OBJECT_SIZE, new ObjectOffset(0), Maps.newHashMap()).get();
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(1), Maps.newHashMap()).get();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals(OBJECT_SIZE + smallObjectLength, blobDescriptor.getByteCount());
    }

    @Test
    public void testAsyncUpdateOnce() throws Exception {
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(0), Maps.newHashMap()).get();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals(smallObjectLength, blobDescriptor.getByteCount());
    }

    @Test
    public void testAsyncGetNonExistentBlob() throws Exception {
        assertFdsError(ErrorCode.MISSING_RESOURCE,
                () -> asyncAm.statBlob(domainName, volumeName, blobName).get());
    }

    private static ConfigurationService.Iface configService;
    private String volumeName;
    private static final int OBJECT_SIZE = 1024 * 1024 * 2;
    private static final int MY_AM_RESPONSE_PORT = 9881;
    private static XdiClientFactory xdiCf;
    private static RealAsyncAm asyncAm;

    private String domainName;
    private String blobName;
    private ByteBuffer bigObject;
    private int smallObjectLength;
    private ByteBuffer smallObject;

    @BeforeClass
    public static void setUpOnce() throws Exception {
        int pmPort = 7000;
        xdiCf = new XdiClientFactory(MY_AM_RESPONSE_PORT);
        configService = xdiCf.remoteOmService("localhost", 9090);
        asyncAm = new RealAsyncAm(xdiCf.remoteOnewayAm("localhost", pmPort+1899), MY_AM_RESPONSE_PORT, 10, TimeUnit.MINUTES);
        asyncAm.start();
    }

    @Before
    public void setUp() throws Exception {
        volumeName = UUID.randomUUID().toString();
        domainName = UUID.randomUUID().toString();
        blobName = UUID.randomUUID().toString();
        bigObject = ByteBufferUtility.randomBytes(OBJECT_SIZE);
        smallObjectLength = 42;
        smallObject = ByteBufferUtility.randomBytes(smallObjectLength);

        configService.createVolume(domainName, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), 0);
    }
}

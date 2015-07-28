package com.formationds.smoketest;

import com.formationds.apis.*;
import com.formationds.commons.Fds;
import com.formationds.hadoop.FdsFileSystem;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.ErrorCode;
import com.formationds.util.ByteBufferUtility;
import com.formationds.xdi.AsyncStreamer;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import com.formationds.xdi.XdiConfigurationApi;
import com.google.common.collect.Maps;
import org.eclipse.jetty.io.ArrayByteBufferPool;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.stream.IntStream;

import static org.junit.Assert.*;


@Ignore
public class AsyncAmTest extends BaseAmTest {
    @Test
    @Ignore
    public void testParallelCreate() throws Exception {
        IntStream.range(0, 10)
                .parallel()
                .map(i -> {
                    try {
                        asyncAm.updateBlobOnce(domainName, volumeName, Integer.toString(i),
                                1, ByteBuffer.allocate(100), 100, new ObjectOffset(1), new HashMap<>()).get();
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                    return 0;
                }).sum();

        IntStream.range(0, 10)
                .parallel()
                .map(i -> {
                    try {
                        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, Integer.toString(i)).get();
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                    return 0;
                }).sum();
    }

    @Test
    public void testOutOfOrderWrites() throws Exception {
        Random random = new Random();
        byte[] halfObject = new byte[OBJECT_SIZE / 2];
        byte[] fullObject = new byte[OBJECT_SIZE];
        random.nextBytes(halfObject);
        random.nextBytes(fullObject);
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, ByteBuffer.wrap(halfObject), OBJECT_SIZE / 2, new ObjectOffset(0), new HashMap<>()).get();
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, ByteBuffer.wrap(halfObject), OBJECT_SIZE / 2, new ObjectOffset(1), new HashMap<>()).get();
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 0, ByteBuffer.wrap(fullObject), OBJECT_SIZE, new ObjectOffset(1), new HashMap<>()).get();
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 0, ByteBuffer.wrap(fullObject), OBJECT_SIZE, new ObjectOffset(0), new HashMap<>()).get();
        for (int i = 0; i < 100; i++) {
            ByteBuffer byteBuffer = asyncAm.getBlob(domainName, volumeName, blobName, OBJECT_SIZE, new ObjectOffset(0)).get();
            assertEquals(OBJECT_SIZE, byteBuffer.remaining());
            byte[] readBuf = new byte[OBJECT_SIZE];
            byteBuffer.get(readBuf);
            assertArrayEquals(fullObject, readBuf);
        }
    }

    @Test
    public void testVolumeMetadata() throws Exception {
        Map<String, String> metadata = asyncAm.getVolumeMetadata(domainName, volumeName).get();
        assertEquals(0, metadata.size());
        metadata.put("hello", "world");
        asyncAm.setVolumeMetadata(domainName, volumeName, metadata).get();
        metadata = asyncAm.getVolumeMetadata(domainName, volumeName).get();
        assertEquals(1, metadata.size());
        assertEquals("world", metadata.get("hello"));
    }

    @Test
    public void testAsyncStreamerWritesOneChunkOnly() throws Exception {
        AsyncStreamer asyncStreamer = new AsyncStreamer(asyncAm, new ArrayByteBufferPool(), new XdiConfigurationApi(configService));
        HashMap<String, String> map = new HashMap<>();
        map.put("hello", "world");
        OutputStream outputStream = asyncStreamer.openForWriting(domainName, volumeName, blobName, map);
        outputStream.write(new byte[OBJECT_SIZE]);
        outputStream.close();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals(OBJECT_SIZE, (long) blobDescriptor.getByteCount());
        assertEquals("world", blobDescriptor.getMetadata().get("hello"));
    }

    @Test
    public void testAsyncStreamer() throws Exception {
        AsyncStreamer asyncStreamer = new AsyncStreamer(asyncAm, new ArrayByteBufferPool(), new XdiConfigurationApi(configService));
        OutputStream outputStream = asyncStreamer.openForWriting(domainName, volumeName, blobName, new HashMap<>());
        outputStream.write(new byte[42]);
        outputStream.write(new byte[42]);
        outputStream.close();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals(84, (long) blobDescriptor.getByteCount());
    }

    @Test
    public void testVolumeContents() throws Exception {
        List<BlobDescriptor> contents = asyncAm.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0, "", BlobListOrder.UNSPECIFIED, false).get();
        assertEquals(0, contents.size());
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(0), metadata).get();
        contents = asyncAm.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0, "", BlobListOrder.UNSPECIFIED, false).get();
        assertEquals(1, contents.size());
    }

    @Test
    public void testVolumeContentsFilter() throws Exception {
        asyncAm.updateBlobOnce(domainName, volumeName, "/", 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), new HashMap<>()).get();
        asyncAm.updateBlobOnce(domainName, volumeName, "/panda", 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), new HashMap<>()).get();
        asyncAm.updateBlobOnce(domainName, volumeName, "/panda/foo", 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), new HashMap<>()).get();
        asyncAm.updateBlobOnce(domainName, volumeName, "/panda/foo/bar", 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), new HashMap<>()).get();
        asyncAm.updateBlobOnce(domainName, volumeName, "/panda/foo/bar/hello", 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), new HashMap<>()).get();
        String filter = "^/[^/]+$";
        List<BlobDescriptor> descriptors = asyncAm.volumeContents(domainName, volumeName, Integer.MAX_VALUE, 0, filter, BlobListOrder.UNSPECIFIED, false).get();
        assertEquals(1, descriptors.size());
    }

    @Test
    @Ignore
    public void testListVolumeContents() throws Exception {
        List<BlobDescriptor> descriptors = asyncAm.volumeContents(domainName, "panda", Integer.MAX_VALUE, 0, "", BlobListOrder.UNSPECIFIED, false).get();
        for (BlobDescriptor descriptor : descriptors) {
            System.out.println(descriptor.getName());
        }
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

    @Test //done 9
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
        assertEquals(1, blobDescriptor.getMetadataSize());
    }

    @Test
    public void testTransactions() throws Exception {
    	// Note: explicit casts added to workaround issues with type inference in Eclipse compiler
        asyncAm.startBlobTx(domainName, volumeName, blobName, 1)
                .thenCompose(tx -> asyncAm.updateBlob(domainName, volumeName, blobName, tx, bigObject, OBJECT_SIZE, new ObjectOffset(0), false).thenApply(x -> tx))
                .thenCompose(tx -> asyncAm.updateBlob(domainName, volumeName, blobName, (TxDescriptor) tx, smallObject, smallObjectLength, new ObjectOffset(1), true).thenApply(x -> tx))
                .thenCompose(tx -> asyncAm.commitBlobTx(domainName, volumeName, blobName, (TxDescriptor) tx))
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
        BlobDescriptor bd1 = asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName,blobName).get();
        assertEquals("world",bd1.getMetadata().get("hello"));

        metadata.put("animal", "panda");
        asyncAm.updateMetadata(domainName, volumeName, blobName, tx, metadata).get();
        asyncAm.commitBlobTx(domainName, volumeName, blobName, tx).get();
        BlobDescriptor  bd2= asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals(2, bd2.getMetadataSize());
        assertEquals("panda", bd2.getMetadata().get("animal"));
        assertEquals(smallObjectLength, bd2.getByteCount());
    }

    @Test
    public void testBlobRename() throws Exception {
        String blobName = UUID.randomUUID().toString();
        String blobName2 = UUID.randomUUID().toString();
        Map<String, String> metadata = new HashMap<>();
        metadata.put("clothing", "boot");
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(0), metadata).get();
        BlobDescriptor bd1 = asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName,blobName).get();
        assertEquals("boot",bd1.getMetadata().get("clothing"));

        // Rename the blob
	try {
            asyncAm.renameBlob(domainName, volumeName, blobName, blobName2).get();
	} catch (ExecutionException e) {
            ApiException apiException = (ApiException) e.getCause();
            assertEquals(ErrorCode.BAD_REQUEST, apiException.getErrorCode());
	}

	//  TODO
	//  reenable this when it works
        // The old one should be gone
//        try {
//            asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).get();
//            fail("Should have gotten an ExecutionException");
//        } catch (ExecutionException e) {
//            ApiException apiException = (ApiException) e.getCause();
//            assertEquals(ErrorCode.MISSING_RESOURCE, apiException.getErrorCode());
//        }
//
//        // The new identical to the old
//        BlobDescriptor bd2 = asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName2).get();
//        assertEquals("boot",bd2.getMetadata().get("clothing"));
    }

    @Test
    public void testMultipleAsyncUpdates() throws Exception {
        String blobName = UUID.randomUUID().toString();
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, bigObject, OBJECT_SIZE, new ObjectOffset(0), Maps.newHashMap()).get();
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(1), Maps.newHashMap()).get();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals(OBJECT_SIZE + smallObjectLength, blobDescriptor.getByteCount());

        byte[] result = new byte[OBJECT_SIZE];
        ByteBuffer byteBuffer = asyncAm.getBlob(FdsFileSystem.DOMAIN,volumeName,blobName,OBJECT_SIZE,new ObjectOffset(0)).get();
        byteBuffer.get(result);
        byte[] bigObjectByteArray = new byte[OBJECT_SIZE];
        bigObject.get(bigObjectByteArray);
        assertArrayEquals(result, bigObjectByteArray);

        byte[] result1 = new byte[smallObjectLength];
        ByteBuffer bb1 = asyncAm.getBlob(FdsFileSystem.DOMAIN,volumeName,blobName,smallObjectLength,new ObjectOffset(1)).get();
        bb1.get(result1);
        byte[] smallObjectByteArray = new byte[smallObjectLength];
        smallObject.get(smallObjectByteArray);
        assertArrayEquals(smallObjectByteArray,result1);

    }

    @Test
    public void testAsyncUpdateOnce() throws Exception {
        asyncAm.updateBlobOnce(domainName, volumeName, blobName, 1, smallObject, smallObjectLength, new ObjectOffset(0), Maps.newHashMap()).get();
        BlobDescriptor blobDescriptor = asyncAm.statBlob(domainName, volumeName, blobName).get();
        assertEquals(smallObjectLength, blobDescriptor.getByteCount());
    }

    @Test //done 6
    public void testAsyncGetNonExistentBlob() throws Exception {
        assertFdsError(ErrorCode.MISSING_RESOURCE,
                () -> asyncAm.statBlob(domainName, volumeName, blobName).get());
    }

    @Test
    public void testStatAndUpdateBlobData() throws ExecutionException, InterruptedException {
        String blobName = "key";
        byte[] buf = new byte[10];
        new Random().nextBytes(buf);

        asyncAm.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.wrap(buf), buf.length, new ObjectOffset(0), new HashMap<>()).get();
        ByteBuffer bb = asyncAm.getBlob(FdsFileSystem.DOMAIN, volumeName, blobName, Integer.MAX_VALUE, new ObjectOffset(0)).get();
        byte[] result = new byte[buf.length];
        bb.get(result);
        assertArrayEquals(buf, result);
        assertEquals(buf.length, (int) asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).get().getByteCount());

        buf = new byte[42];
        new Random().nextBytes(buf);
        TxDescriptor tx = asyncAm.startBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, 1).get();
        asyncAm.updateBlob(FdsFileSystem.DOMAIN, volumeName, blobName, tx, ByteBuffer.wrap(buf), buf.length, new ObjectOffset(0),false).get();
        asyncAm.commitBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, tx).get();

        bb = asyncAm.getBlob(FdsFileSystem.DOMAIN, volumeName, blobName, Integer.MAX_VALUE, new ObjectOffset(0)).get();
        result = new byte[buf.length];
        bb.get(result);
        assertArrayEquals(buf, result);
        assertEquals(buf.length, (int) asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).get().getByteCount());
    }

    @Test
    public void testVolumeContentsOnMissingVolume() throws Exception {
        assertFdsError(ErrorCode.MISSING_RESOURCE,
                () -> asyncAm.volumeContents(FdsFileSystem.DOMAIN,"nonExistingVolume",Integer.MAX_VALUE,0,"",BlobListOrder.LEXICOGRAPHIC,false).get());
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
        xdiCf = new XdiClientFactory();
        configService = xdiCf.remoteOmService(Fds.getFdsHost(), 9090);
        asyncAm = new RealAsyncAm(Fds.getFdsHost(), 8899, MY_AM_RESPONSE_PORT, 10, TimeUnit.MINUTES);
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

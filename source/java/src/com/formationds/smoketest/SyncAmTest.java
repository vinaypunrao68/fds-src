package com.formationds.smoketest;

import com.formationds.apis.ErrorCode;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.hadoop.FdsFileSystem;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import org.junit.Ignore;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.*;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

@Ignore
public class SyncAmTest extends BaseAmTest {
    @Test
    public void testTransaction() throws Exception {
        String blobName = "key";
        byte[] first = new byte[OBJECT_SIZE];
        new Random().nextBytes(first);
        amService.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.wrap(first), OBJECT_SIZE, new ObjectOffset(0), new HashMap<>());

        int arbitrarySize = 42;
        byte[] second = new byte[arbitrarySize];
        new Random().nextBytes(second);
        TxDescriptor tx = amService.startBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, 1);
        amService.updateBlob(FdsFileSystem.DOMAIN, volumeName, blobName, tx, ByteBuffer.wrap(second), arbitrarySize, new ObjectOffset(0), true);
        amService.commitBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, tx);

        byte[] result = new byte[arbitrarySize];
        ByteBuffer bb = amService.getBlob(FdsFileSystem.DOMAIN, volumeName, blobName, arbitrarySize, new ObjectOffset(0));
        bb.get(result);
        assertArrayEquals(second, result);
    }

    @Test
    public void testUpdateOnceSeveralTimes() throws Exception {
        String blobName = "key";
        byte[] first = new byte[OBJECT_SIZE];
        byte[] second = new byte[OBJECT_SIZE];
        new Random().nextBytes(first);
        new Random().nextBytes(second);
        amService.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.wrap(first), OBJECT_SIZE, new ObjectOffset(0), new HashMap<>());
        amService.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.wrap(second), OBJECT_SIZE, new ObjectOffset(1), new HashMap<>());
        assertEquals(2 * OBJECT_SIZE, amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).getByteCount());

        byte[] result = new byte[OBJECT_SIZE];

        ByteBuffer bb = amService.getBlob(FdsFileSystem.DOMAIN, volumeName, blobName, OBJECT_SIZE, new ObjectOffset(0));
        bb.get(result);
        assertArrayEquals(first, result);

        bb = amService.getBlob(FdsFileSystem.DOMAIN, volumeName, blobName, OBJECT_SIZE, new ObjectOffset(1));
        bb.get(result);
        assertArrayEquals(second, result);
    }

    @Test
    public void testDeleteBlob() throws Exception {
        String blobName = "key";
        byte[] buf = new byte[10];
        new Random().nextBytes(buf);

        amService.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.wrap(buf), buf.length, new ObjectOffset(0), new HashMap<>());
        assertEquals(buf.length, (int) amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).getByteCount());
        amService.deleteBlob(FdsFileSystem.DOMAIN, volumeName, blobName);

        assertFdsError(ErrorCode.MISSING_RESOURCE,
                () -> amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName));
    }

    @Test
    public void testStatAndUpdateBlobData() throws Exception {
        String blobName = "key";
        byte[] buf = new byte[10];
        new Random().nextBytes(buf);

        amService.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.wrap(buf), buf.length, new ObjectOffset(0), new HashMap<>());
        ByteBuffer bb = amService.getBlob(FdsFileSystem.DOMAIN, volumeName, blobName, Integer.MAX_VALUE, new ObjectOffset(0));
        byte[] result = new byte[buf.length];
        bb.get(result);
        assertArrayEquals(buf, result);
        assertEquals(buf.length, (int) amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).getByteCount());

        buf = new byte[42];
        new Random().nextBytes(buf);
        TxDescriptor tx = amService.startBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, 1);
        amService.updateBlob(FdsFileSystem.DOMAIN, volumeName, blobName, tx, ByteBuffer.wrap(buf), buf.length, new ObjectOffset(0), true);
        amService.commitBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, tx);

        bb = amService.getBlob(FdsFileSystem.DOMAIN, volumeName, blobName, Integer.MAX_VALUE, new ObjectOffset(0));
        result = new byte[buf.length];
        bb.get(result);
        assertArrayEquals(buf, result);
        assertEquals(buf.length, (int) amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).getByteCount());
    }

    @Test
    public void testStatAndUpdateBlobMetadata() throws Exception {
        String blobName = "key";
        HashMap<String, String> metadata = new HashMap<>();
        byte[] buf = new byte[10];
        new Random().nextBytes(buf);

        amService.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.wrap(buf), buf.length, new ObjectOffset(0), metadata);
        BlobDescriptor bd = amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName);
        assertEquals(buf.length, bd.getByteCount());

        metadata.put("hello", "world");
        TxDescriptor tx = amService.startBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, 1);
        amService.updateMetadata(FdsFileSystem.DOMAIN, volumeName, blobName, tx, metadata);
        amService.commitBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, tx);
        bd = amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName);
        assertEquals(1, bd.getMetadataSize());
        assertEquals(buf.length, (long) bd.getByteCount());
        assertEquals("world", bd.getMetadata().get("hello"));
    }

    @Test
    public void testStatInexistentBlob() throws Exception {
        assertFdsError(ErrorCode.MISSING_RESOURCE,
                () -> amService.statBlob(FdsFileSystem.DOMAIN, volumeName, "nonExistent"));
    }

    @Test
    public void testVolumeContents() throws Exception {
        List<BlobDescriptor> result = amService.volumeContents(FdsFileSystem.DOMAIN, volumeName, Integer.MAX_VALUE, 0, "", BlobListOrder.LEXICOGRAPHIC, false);
        assertEquals(0, result.size());
        amService.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, "key", 1, ByteBuffer.allocate(10), 10, new ObjectOffset(0), new HashMap<>());
        result = amService.volumeContents(FdsFileSystem.DOMAIN, volumeName, Integer.MAX_VALUE, 0, "", BlobListOrder.LEXICOGRAPHIC, false);
        assertEquals(1, result.size());
    }

    @Test
    public void testVolumeContentsOnMissingVolume() throws Exception {
        assertFdsError(ErrorCode.MISSING_RESOURCE,
                () -> amService.volumeContents(FdsFileSystem.DOMAIN, "nonExistent", Integer.MAX_VALUE, 0, "", BlobListOrder.LEXICOGRAPHIC, false));
    }

    @Test
    public void testUpdateMetadata() throws Exception {
        Map<String, String> metadata = new HashMap<>();
        metadata.put("first", "firstValue");
        String blobName = UUID.randomUUID().toString();
        byte[] bytes = new byte[42];
        amService.updateBlobOnce(FdsFileSystem.DOMAIN, volumeName, blobName, 1, ByteBuffer.wrap(bytes), bytes.length, new ObjectOffset(0), metadata);

        // First statBlob after creation
        BlobDescriptor bd = amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName);
        assertEquals(bd.getMetadata().get("first"), "firstValue");
        assertEquals(bytes.length, bd.getByteCount());

        // Update metadata
        metadata.put("second", "secondValue");
        TxDescriptor tx = amService.startBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, 0);
        amService.updateMetadata(FdsFileSystem.DOMAIN, volumeName, blobName, tx, metadata);
        amService.commitBlobTx(FdsFileSystem.DOMAIN, volumeName, blobName, tx);

        // Assert metadata visible and consistent
        bd = amService.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName);
        assertEquals(bd.getMetadata().get("first"), "firstValue");
        assertEquals(bd.getMetadata().get("second"), "secondValue");
        // assertEquals(bytes.length, bd.getByteCount());
    }


    @Test
    public void testAsyncGetNonExistentBlob() throws Exception {
        String blobName = UUID.randomUUID().toString();
        assertFdsError(ErrorCode.MISSING_RESOURCE,
                () -> asyncAm.statBlob(FdsFileSystem.DOMAIN, volumeName, blobName).get());
    }
}

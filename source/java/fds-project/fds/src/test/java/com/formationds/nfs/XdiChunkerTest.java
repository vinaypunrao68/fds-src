package com.formationds.nfs;

import com.formationds.xdi.contracts.api.obj.*;
import com.formationds.xdi.contracts.api.stub.XdiStub;
import org.junit.Before;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.Random;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.atomic.AtomicLong;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class XdiChunkerTest {
    public static final int MAX_OBJECT_SIZE = 1024;
    public static final BlobPath PATH = new BlobPath(42, "blobName");
    private XdiChunker chunker;
    private CountingXdi countingXdi;

    @Test
    public void testNoReadsForAlignedWrites() throws Exception {
        byte[] buf = new byte[MAX_OBJECT_SIZE];
        new Random().nextBytes(buf);
        chunker.write(PATH, MAX_OBJECT_SIZE, buf, 0, MAX_OBJECT_SIZE, x -> null);
        assertEquals(0, countingXdi.objectReads.get());
        assertEquals(1, countingXdi.objectWrites.get());
        byte[] dest = new byte[MAX_OBJECT_SIZE];
        int read = chunker.read(PATH, MAX_OBJECT_SIZE, MAX_OBJECT_SIZE, dest, 0, MAX_OBJECT_SIZE);
        assertEquals(MAX_OBJECT_SIZE, read);
        assertArrayEquals(buf, dest);
    }

    @Test
    public void testReadWrite() throws Exception {
        readWriteCycle(42);
        readWriteCycle(84);
        readWriteCycle(1025);
        readWriteCycle(1024);
        readWriteCycle(2049);
    }

    @Test
    public void testReadWayWayPastEnd() throws Exception {
        assertEquals(0, chunker.read(PATH, 10, 1048576, new byte[4096], 21473722368l, 4096));
    }

    @Test
    public void testReadPastEnd() throws Exception {
        int length = MAX_OBJECT_SIZE * 2;
        byte[] buf = new byte[length];
        new Random().nextBytes(buf);
        chunker.write(PATH, MAX_OBJECT_SIZE, buf, 0, length, x -> null);
        byte[] dest = new byte[10];
        int read = chunker.read(PATH, MAX_OBJECT_SIZE, MAX_OBJECT_SIZE, dest, MAX_OBJECT_SIZE * 2, 10);
        assertEquals(0, read);
        read = chunker.read(PATH, MAX_OBJECT_SIZE, 1048576, new byte[4096], 21473722368l, 4096);
        assertEquals(0, read);
    }

    @Test
    public void testUnalignedReads() throws Exception {
        int length = MAX_OBJECT_SIZE * 2;
        byte[] buf = new byte[length];
        new Random().nextBytes(buf);
        chunker.write(PATH, MAX_OBJECT_SIZE, buf, 0, length, x -> null);
        byte[] dest = new byte[10];
        int read = chunker.read(PATH, length, MAX_OBJECT_SIZE, dest, MAX_OBJECT_SIZE - 5, 10);
        assertEquals(10, read);
    }

    private void readWriteCycle(int length) throws Exception {
        byte[] bytes = randomBytes(length);
        String arbitraryValue = UUID.randomUUID().toString();
        chunker.write(PATH, MAX_OBJECT_SIZE, bytes, 0, length, meta -> meta.put("key", arbitraryValue));
        ObjectRange objectRange = new ObjectRange(0, Math.floorDiv((long) length, (long) MAX_OBJECT_SIZE));
        Blob blob = countingXdi.readBlob(new ReadBlobRequest(PATH, new CachePolicy(true, false), objectRange)).get();
        assertEquals(arbitraryValue, blob.getMetadata().get("key"));
        byte[] readBuf = new byte[length];
        chunker.read(PATH, length, MAX_OBJECT_SIZE, readBuf, 0, length);
        assertArrayEquals(bytes, readBuf);
        chunker.read(PATH, length, MAX_OBJECT_SIZE, readBuf, 0, length);
        assertArrayEquals(bytes, readBuf);
    }

    private static Random RNG = new Random();

    @Before
    public void setUp() throws Exception {
        countingXdi = new CountingXdi();
        countingXdi.setVolume(PATH.getVolumeId(), "FOO", MAX_OBJECT_SIZE);
        chunker = new XdiChunker(countingXdi);
        Blob blob = new Blob(PATH);
        countingXdi.writeBlob(new WriteBlobRequest(blob, new CachePolicy(true, true), new ObjectRange(0, 0)));
    }

    private byte[] randomBytes(int length) {
        byte[] bytes = new byte[length];
        RNG.nextBytes(bytes);
        return bytes;
    }

    private static class CountingXdi extends XdiStub {
        final AtomicLong objectReads;
        final AtomicLong objectWrites;

        public CountingXdi() {
            super();
            objectReads = new AtomicLong(0);
            objectWrites = new AtomicLong(0);
        }

        @Override
        public CompletableFuture<ByteBuffer> readObject(String objectId) {
            return super.readObject(objectId)
                    .whenComplete((bb, e) -> objectReads.incrementAndGet());
        }

        @Override
        public CompletableFuture<String> writeObject(WriteObjectRequest request) {
            return super.writeObject(request)
                    .whenComplete((bb, e) -> objectWrites.incrementAndGet());
        }
    }
}

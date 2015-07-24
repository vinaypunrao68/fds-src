package com.formationds.util.s3.auth;

import org.apache.commons.codec.binary.Hex;
import org.junit.Before;
import org.junit.Test;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Optional;
import java.util.Random;
import java.util.stream.Collectors;

import static org.junit.Assert.*;

public class ChunkDecoderTest {

    @Before
    public void setUp() {

    }

    @Test
    public void testSimple() throws Exception {
        byte[] data = testCase1Raw();
        ByteBuffer buffer = ByteBuffer.wrap(data);

        ChunkDecoder decoder = new ChunkDecoder();
        // Assert that the initial state is that no chunks are available
        assertEquals(Optional.empty(), decoder.getNextAvailableChunk());
        assertTrue(!decoder.isCurrentlyDecoding());
        decoder.input(buffer);
        assertTrue(!decoder.isCurrentlyDecoding());

        // Assert the entire buffer is depleted
        assertEquals(0, buffer.remaining());

        testCase1Validate(decoder);


    }

    @Test
    public void testTinyBuffers() throws Exception {
        byte[] data = testCase1Raw();
        ChunkDecoder decoder = new ChunkDecoder();

        assertTrue(!decoder.isCurrentlyDecoding());
        for(int i = 0; i < data.length; i++) {
            decoder.input(ByteBuffer.wrap(data, i, 1));
        }
        assertTrue(!decoder.isCurrentlyDecoding());

        testCase1Validate(decoder);
    }

    public byte[] makeSignature(int i) {
        byte[] result = new byte[32];
        ByteBuffer buf = ByteBuffer.wrap(result);
        buf.putInt(i);
        buf.putInt(~i);
        buf.putInt(i);
        buf.putInt(~i);

        return result;
    }

    public byte[] testCase1Raw() throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();

        writeChunk(baos, makeChunkData(1024, 1), makeSignature(1));
        writeChunk(baos, makeChunkData(1024, 2), makeSignature(2));
        writeChunk(baos, makeChunkData(0, 3), makeSignature(3));

        return baos.toByteArray();
    }

    public void testCase1Validate(ChunkDecoder decoder) {
        Optional<ChunkData> chunk1 = decoder.getNextAvailableChunk();
        Optional<ChunkData> chunk2 = decoder.getNextAvailableChunk();
        Optional<ChunkData> chunk3 = decoder.getNextAvailableChunk();
        Optional<ChunkData> chunk4 = decoder.getNextAvailableChunk();

        assertTrue(chunk1.isPresent());
        assertTrue(chunk2.isPresent());
        assertTrue(chunk3.isPresent());
        assertEquals(Optional.empty(), chunk4);

        assertArrayEquals(makeChunkData(1024, 1), consolidateChunkBuffers(chunk1.get().getChunkBuffers()));
        assertArrayEquals(makeChunkData(1024, 2), consolidateChunkBuffers(chunk2.get().getChunkBuffers()));
        assertArrayEquals(new byte[] {}, consolidateChunkBuffers(chunk3.get().getChunkBuffers()));

        assertArrayEquals(makeSignature(1), chunk1.get().getHeaderSignature());
        assertArrayEquals(makeSignature(2), chunk2.get().getHeaderSignature());
        assertArrayEquals(makeSignature(3), chunk3.get().getHeaderSignature());
    }


    private String makeHeader(int bytes, byte[] signature) {
        return Integer.toHexString(bytes) + ";chunk-signature=" + Hex.encodeHexString(signature) + "\r\n";
    }

    private byte[] makeChunkData(int size, long seed) {
        Random r = new Random(seed);
        byte[] bytes = new byte[size];
        r.nextBytes(bytes);
        return bytes;
    }

    private void writeChunk(OutputStream stream, byte[] data, byte[] signature) throws IOException {
        String header = makeHeader(data.length, signature);
        stream.write(header.getBytes());

        stream.write(data);
        stream.write(new byte[] { '\r', '\n' });
    }

    private byte[] consolidateChunkBuffers(List<ByteBuffer> buffers) {
        int length = buffers.stream().collect(Collectors.summingInt(i -> i.remaining()));
        byte[] bytes = new byte[length];
        ByteBuffer dst = ByteBuffer.wrap(bytes);
        for(ByteBuffer buf : buffers)
            dst.put(buf.slice());

        return bytes;
    }

}
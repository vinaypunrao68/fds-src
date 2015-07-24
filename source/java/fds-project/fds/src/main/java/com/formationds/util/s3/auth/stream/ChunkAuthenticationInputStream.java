package com.formationds.util.s3.auth.stream;

import com.formationds.util.s3.auth.ChunkData;
import com.formationds.util.s3.auth.ChunkDecoder;
import com.formationds.util.s3.auth.ChunkDecodingException;
import com.formationds.util.s3.auth.ChunkSignatureSequence;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.Optional;
import java.util.Queue;
import java.util.stream.Collectors;

public class ChunkAuthenticationInputStream extends InputStream {
    private final ChunkSignatureSequence signatureSequence;
    private InputStream wrapped;
    private ChunkDecoder decoder;
    private byte[] currentChunkSignature;
    private Queue<ByteBuffer> chunkBuffers;
    private boolean signatureMismatch;

    public ChunkAuthenticationInputStream(InputStream wrapped, ChunkSignatureSequence seq) {
        this.wrapped = wrapped;
        decoder = new ChunkDecoder();
        chunkBuffers = new LinkedList<>();
        signatureSequence = seq;
        currentChunkSignature = seq.getSeedSignature();
    }

    private ByteBuffer getCurrentBuffer() throws IOException {
        if(signatureMismatch)
            throw new SecurityException();

        while(!chunkBuffers.isEmpty() && chunkBuffers.peek().remaining() == 0)
            chunkBuffers.remove();

        if(chunkBuffers.isEmpty()) {
            Optional<ChunkData> chunk = decoder.getNextAvailableChunk();
            while(!chunk.isPresent()) {
                int size = wrapped.available() > 0 ? wrapped.available() : 4096;
                byte[] data = new byte[size];
                int read = wrapped.read(data);
                if(read == -1)
                    break;

                try {
                    decoder.input(ByteBuffer.wrap(data, 0, read));
                } catch (ChunkDecodingException e) {
                    throw new IOException("failure while decoding chunks", e);
                }

                chunk = decoder.getNextAvailableChunk();
            }

            if(chunk.isPresent()) {
                ChunkData currentChunk = chunk.get();
                byte[] headerSignature = currentChunk.getHeaderSignature();
                byte[] computed = signatureSequence.computeNextSignature(currentChunkSignature, currentChunk.getChunkDigest());
                if(!Arrays.equals(headerSignature, computed)) {
                    signatureMismatch = true;
                    throw new SecurityException();
                }

                currentChunkSignature = computed;
                chunkBuffers.addAll(currentChunk.getChunkBuffers());
            }
        }

        return chunkBuffers.peek();

    }

    @Override
    public int read() throws IOException {
        ByteBuffer current = getCurrentBuffer();
        if(current != null)
            return current.get();

        return -1;
    }

    @Override
    public int read(byte[] b) throws IOException {
        ByteBuffer current = getCurrentBuffer();
        if(current != null) {
            int length = Math.min(b.length, current.remaining());
            current.get(b, 0, length);
            return length;
        }

        return - 1;
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        ByteBuffer current = getCurrentBuffer();
        if(current != null) {
            int length = Math.min(len, current.remaining());
            current.get(b, off, length);
            return length;
        }

        return -1;
    }

    @Override
    public void close() throws IOException {
        wrapped.close();
    }

    @Override
    public int available() throws IOException {
        return chunkBuffers.stream().collect(Collectors.summingInt(c -> c.remaining()));
    }
}

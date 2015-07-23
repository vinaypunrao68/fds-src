package com.formationds.util.s3.auth;

import com.formationds.util.DigestUtil;
import org.apache.commons.codec.DecoderException;
import org.apache.commons.codec.binary.Hex;

import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

public class ChunkData {
    StringBuilder headerAccumulator = new StringBuilder();
    char lastHeaderChar = 0;

    private ChunkHeaderData headerData;
    private int chunkBytesRemaining;
    private int chunkEndBytesRemaining;
    private MessageDigest digest;
    private byte[] chunkSha256;

    private List<ByteBuffer> chunkBuffers;

    ChunkData() {
        headerData = null;
        chunkBytesRemaining = -1;
        chunkEndBytesRemaining = -1;
        chunkBuffers = new ArrayList<>();
        chunkSha256 = null;
        digest = DigestUtil.newSha256();
    }

    public void read(ByteBuffer buffer) throws ChunkDecodingException {
        while (buffer.remaining() > 0 && !chunkComplete()) {
            if (headerData == null) {
                char c = (char)buffer.get();
                headerAccumulator.append(c);

                if(headerAccumulator.length() > 1000)
                    throw new ChunkDecodingException("malformed or missing chunk header - read 1000 characters without seeing the chunk header terminate");

                if (lastHeaderChar == '\r' && c == '\n') {
                    headerData = parseHeaderData(headerAccumulator.toString());
                    chunkBytesRemaining = headerData.length;
                    chunkEndBytesRemaining = 2;
                }
                lastHeaderChar = c;
            } else if(chunkBytesRemaining > 0) {
                int readLength = Math.min(buffer.remaining(), chunkBytesRemaining);
                ByteBuffer buf = buffer.slice();
                buf.limit(readLength);
                chunkBuffers.add(buf);
                chunkBytesRemaining -= readLength;
                digest.update(buf.slice());
                buffer.position(buffer.position() + readLength);
            } else if(chunkEndBytesRemaining > 0) {
                if(chunkSha256 == null)
                    chunkSha256 = digest.digest();
                int readLength = Math.min(buffer.remaining(), chunkEndBytesRemaining);
                // do we need to validate that the characters at the end are \r\n?
                buffer.position(buffer.position() + readLength);
                chunkEndBytesRemaining -= readLength;
            }
        }
    }

    private ChunkHeaderData parseHeaderData(String header) throws ChunkDecodingException {
        try {
            if(!header.endsWith("\r\n"))
                throw new ChunkDecodingException("chunk read failed, malformed chunk header - header terminator is not correct");

            header = header.substring(0, header.length() - 2);
            String[] chunkSignatureParts = header.split(";");

            if (chunkSignatureParts.length != 2)
                throw new ChunkDecodingException("chunk read failed, malformed chunk header");

            int length = 0;
            try {
                length = Integer.parseInt(chunkSignatureParts[0], 16); // parse this as hex
            } catch (NumberFormatException ex) {
                throw new ChunkDecodingException("chunk read failed, malformed chunk header - expecting integer as first part of header", ex);
            }

            String signature = chunkSignatureParts[1];
            String[] signatureParts = signature.split("=");

            if (signatureParts.length != 2 || !signatureParts[0].equals("chunk-signature"))
                throw new ChunkDecodingException("chunk read failed, malformed chunk header - expecting 'chunk-signature=VALUE");


            byte[] signatureBytes = Hex.decodeHex(signatureParts[1].toCharArray());

            return new ChunkHeaderData(length, signatureBytes);
        } catch(ChunkDecodingException ex) {
            throw ex;
        } catch (DecoderException e) {
            throw new ChunkDecodingException("chunk read failed, chunk signature could not be decoded", e);
        }
    }

    public boolean chunkComplete() {
        return headerData != null && chunkBytesRemaining == 0 && chunkEndBytesRemaining == 0;
    }

    public byte[] getHeaderSignature() {
        validateChunkComplete();

        return headerData.signature;
    }

    public byte[] getChunkDigest() {
        validateChunkComplete();

        return chunkSha256;
    }

    private void validateChunkComplete() {
        if(!chunkComplete())
            throw new IllegalStateException("chunk has not completed reading");
    }

    public List<ByteBuffer> getChunkBuffers() {
        validateChunkComplete();

        return chunkBuffers.stream().map(b -> b.slice()).collect(Collectors.toList());
    }

    public long getTotalLength() {
        return chunkBuffers.stream().collect(Collectors.summingLong(b -> b.remaining()));
    }

    class ChunkHeaderData {
        byte[] signature;
        int length;

        public ChunkHeaderData(int length, byte[] signature) {
            this.length = length;
            this.signature = signature;
        }
    }
}

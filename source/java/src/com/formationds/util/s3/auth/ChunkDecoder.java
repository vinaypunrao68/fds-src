package com.formationds.util.s3.auth;

import java.nio.ByteBuffer;
import java.util.*;

public class ChunkDecoder {
    private Queue<ByteBuffer> inputData;
    private Queue<ChunkData> outputData;
    private ChunkData currentChunk;

    public ChunkDecoder() {
        inputData = new LinkedList<>();
        outputData = new LinkedList<>();
    }

    public void input(ByteBuffer input) throws ChunkDecodingException {
        while(input.remaining() > 0) {
            if(currentChunk == null)
                currentChunk = new ChunkData();

            currentChunk.read(input);

            if(currentChunk.chunkComplete()) {
                outputData.add(currentChunk);
                currentChunk = null;
            }
        }
    }

    public Optional<ChunkData> getNextAvailableChunk() {
        if(!outputData.isEmpty()) {
            return Optional.of(outputData.poll());
        } else {
            return Optional.empty();
        }
    }

    public boolean isCurrentlyDecoding() {
        return currentChunk != null;
    }
}

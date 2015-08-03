package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import org.apache.log4j.Logger;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Map;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class ChunkyAm implements Chunker.ChunkyIo {
    private static final Logger LOG = Logger.getLogger(ChunkyAm.class);
    private AsyncAm asyncAm;

    public ChunkyAm(AsyncAm asyncAm) {
        this.asyncAm = asyncAm;
    }

    @Override
    public ByteBuffer read(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset) throws Exception {
        try {
            ByteBuffer byteBuffer = unwindExceptions(() -> asyncAm.getBlob(BlockyVfs.DOMAIN, volume, blobName, objectSize, objectOffset).get());
            return byteBuffer;
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                throw new FileNotFoundException();
            } else {
                throw e;
            }
        }
    }

    @Override
    public void write(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, boolean isEndOfBlob, Map<String, String> metadata) throws Exception {
        // This would cause DM to crash
        if (!isEndOfBlob && byteBuffer.remaining() != objectSize) {
            String message = "All objects except the last one should be exactly MAX_OBJECT_SIZE long";
            LOG.debug(message);
            throw new IOException(message);
        }

        try {
            long then = System.currentTimeMillis();
            int length = byteBuffer.remaining();
            unwindExceptions(() -> asyncAm.updateBlobOnce(domain,
                    volume,
                    blobName,
                    isEndOfBlob ? 1 : 0,
                    byteBuffer,
                    length,
                    objectOffset,
                    metadata).get());
            long elapsed = System.currentTimeMillis() - then;
        } catch (Exception e) {
            LOG.error("AmIO.write() - updateBlobOnce() failed", e);
            throw e;
        }
    }
}

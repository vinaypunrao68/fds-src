package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import org.apache.log4j.Logger;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class AmIO implements Chunker.ChunkIo {
    private static final Logger LOG = Logger.getLogger(AmIO.class);
    private AsyncAm asyncAm;

    public AmIO(AsyncAm asyncAm) {
        this.asyncAm = asyncAm;
    }

    @Override
    public ByteBuffer read(NfsPath path, int objectSize, ObjectOffset objectOffset) throws Exception {
        try {
            ByteBuffer byteBuffer = unwindExceptions(() -> asyncAm.getBlob(AmVfs.DOMAIN, path.getVolume(), path.blobName(), objectSize, objectOffset).get());
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
    public void write(NfsEntry entry, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, boolean isEndOfBlob) throws Exception {
        LOG.debug("AmIO.write(): " + entry.path().toString() + ", objectSize=" + objectSize + ", objectOffset=" + objectOffset.getValue() +
                ", byteBuffer=" + byteBuffer.remaining() + "bytes, isEndOfBlob=" + isEndOfBlob);

        // This would cause DM to crash
        if (!isEndOfBlob && byteBuffer.remaining() != objectSize) {
            String message = "All objects except the last one should be exactly MAX_OBJECT_SIZE long";
            LOG.debug(message);
            throw new IOException(message);
        }

        try {
            long then = System.currentTimeMillis();
            int length = byteBuffer.remaining();
            unwindExceptions(() -> asyncAm.updateBlobOnce(AmVfs.DOMAIN,
                    entry.path().getVolume(),
                    entry.path().blobName(),
                    isEndOfBlob ? 1 : 0,
                    byteBuffer,
                    length,
                    objectOffset,
                    entry.attributes().asMetadata()).get());
            long elapsed = System.currentTimeMillis() - then;

            LOG.debug("AM wrote " + length + " bytes in " + elapsed + " ms");
        } catch (Exception e) {
            LOG.error("AmIO.write() - updateBlobOnce() failed", e);
            throw e;
        }
    }
}

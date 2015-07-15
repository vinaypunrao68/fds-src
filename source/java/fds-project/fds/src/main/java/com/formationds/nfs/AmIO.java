package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import org.apache.log4j.Logger;

import java.io.FileNotFoundException;
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
            return unwindExceptions(() -> asyncAm.getBlob(AmVfs.DOMAIN, path.getVolume(), path.blobName(), objectSize, objectOffset).get());
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                throw new FileNotFoundException();
            } else {
                throw e;
            }
        }
    }

    @Override
    public void write(NfsEntry entry, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer) throws Exception {
        String message = entry.path().toString() + ", objectSize=" + objectSize + ", objectOffset=" + objectOffset.getValue() + ", byteBuffer=" + byteBuffer.remaining() + "bytes";
        LOG.debug("AmIO.write(): " + message);
        try {
            unwindExceptions(() ->
                    asyncAm.updateBlobOnce(AmVfs.DOMAIN,
                            entry.path().getVolume(),
                            entry.path().blobName(),
                            1,
                            byteBuffer,
                            byteBuffer.remaining(),
                            objectOffset,
                            entry.attributes().asMetadata()).get());
        } catch (Exception e) {
            LOG.error("AmIO.write() - updateBlobOnce() " + message, e);
            throw e;
        }
    }
}

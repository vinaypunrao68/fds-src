package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;

import java.io.FileNotFoundException;
import java.nio.ByteBuffer;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class AmIO implements Chunker.ChunkIo {
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
    public void write(NfsPath path, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer) throws Exception {
        BlobDescriptor blobDescriptor =
                unwindExceptions(() -> asyncAm.statBlob(AmVfs.DOMAIN, path.getVolume(), path.blobName()).get());

        NfsAttributes attributes = new NfsAttributes(blobDescriptor)
                .updateMtime()
                .updateSize(objectOffset.getValue() * objectSize + byteBuffer.remaining());

        unwindExceptions(() ->
                asyncAm.updateBlobOnce(AmVfs.DOMAIN,
                        path.getVolume(),
                        path.blobName(),
                        1,
                        byteBuffer,
                        byteBuffer.remaining(),
                        objectOffset,
                        attributes.asMetadata()).get());
    }
}

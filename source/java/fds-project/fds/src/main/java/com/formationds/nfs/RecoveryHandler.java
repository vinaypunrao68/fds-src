package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.IoSupplier;
import com.formationds.xdi.RecoverableException;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public class RecoveryHandler implements IoOps {
    private static final Logger LOG = Logger.getLogger(RecoveryHandler.class);
    private final IoOps ops;
    private int retryCount;
    private Duration retryInterval;

    public RecoveryHandler(IoOps ops, int retryCount, Duration retryInterval) {
        this.ops = ops;
        this.retryCount = retryCount;
        this.retryInterval = retryInterval;
    }

    @Override
    public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException {
        return attempt(() -> ops.readMetadata(domain, volumeName, blobName));
    }

    @Override
    public void writeMetadata(String domain, String volumeName, String blobName, Map<String, String> metadata, boolean deferrable) throws IOException {
        attempt(() -> {
            ops.writeMetadata(domain, volumeName, blobName, metadata, deferrable);
            return null;
        });
    }

    @Override
    public ByteBuffer readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int objectSize) throws IOException {
        return attempt(() -> ops.readCompleteObject(domain, volumeName, blobName, objectOffset, objectSize));
    }

    @Override
    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, ByteBuffer byteBuffer, int objectSize, boolean deferrable) throws IOException {
        attempt(() -> {
            ops.writeObject(domain, volumeName, blobName, objectOffset, byteBuffer, objectSize, deferrable);
            return null;
        });
    }

    @Override
    public void deleteBlob(String domain, String volumeName, String blobName) throws IOException {
        attempt(() -> {
            ops.deleteBlob(domain, volumeName, blobName);
            return null;
        });
    }

    @Override
    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException {
        attempt(() -> {
            ops.renameBlob(domain, volumeName, oldName, newName);
            return null;
        });
    }

    @Override
    public List<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException {
        return attempt(() -> ops.scan(domain, volume, blobNamePrefix));
    }

    @Override
    public void flush() throws IOException {
        attempt(() -> {
            ops.flush();
            return null;
        });
    }

    private <T> T attempt(IoSupplier<T> supplier) throws IOException {
        for (int i = 0; i < retryCount; i++) {
            try {
                return supplier.supply();
            } catch (Exception e) {
                if (e instanceof RecoverableException) {
                    try {
                        Thread.sleep(retryInterval.toMillis());
                    } catch (InterruptedException e1) {
                        break;
                    }
                    continue;
                } else {
                    throw e;
                }
            }
        }
        String message = "Invocation failed after " + retryCount + " attempts, aborting";
        IOException e = new IOException(message);
        LOG.error(message, e);
        throw e;
    }
}

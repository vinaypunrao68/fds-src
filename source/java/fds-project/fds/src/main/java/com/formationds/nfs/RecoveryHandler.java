package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.IoSupplier;
import com.formationds.xdi.RecoverableException;
import org.apache.log4j.Logger;
import org.joda.time.Duration;

import java.io.IOException;
import java.util.Collection;
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
    public void writeMetadata(String domain, String volumeName, String blobName, Map<String, String> metadata) throws IOException {
        attempt(() -> {
            ops.writeMetadata(domain, volumeName, blobName, metadata);
            return null;
        });
    }

    @Override
    public void commitMetadata(String domain, String volumeName, String blobName) throws IOException {
        attempt(() -> {
            ops.commitMetadata(domain, volumeName, blobName);
            return null;
        });
    }

    @Override
    public FdsObject readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int maxObjectSize) throws IOException {
        return attempt(() -> ops.readCompleteObject(domain, volumeName, blobName, objectOffset, maxObjectSize));
    }

    @Override
    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, FdsObject fdsObject) throws IOException {
        attempt(() -> {
            ops.writeObject(domain, volumeName, blobName, objectOffset, fdsObject);
            return null;
        });
    }

    @Override
    public void commitObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset) throws IOException {
        attempt(() -> {
            ops.commitObject(domain, volumeName, blobName, objectOffset);
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
    public Collection<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException {
        return attempt(() -> ops.scan(domain, volume, blobNamePrefix));
    }

    @Override
    public void commitAll() throws IOException {
        attempt(() -> {
            ops.commitAll();
            return null;
        });
    }

    @Override
    public void onVolumeDeletion(String domain, String volumeName) throws IOException {
        ops.onVolumeDeletion(domain, volumeName);
    }

    private <T> T attempt(IoSupplier<T> supplier) throws IOException {
        for (int i = 0; i < retryCount; i++) {
            try {
                return supplier.supply();
            } catch (Exception e) {
                if (e instanceof RecoverableException) {
                    try {
                        Thread.sleep(retryInterval.getMillis());
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

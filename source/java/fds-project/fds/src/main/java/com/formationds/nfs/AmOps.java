package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.protocol.*;
import com.formationds.xdi.AsyncAm;
import org.apache.log4j.Logger;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class AmOps implements IoOps {
    private static final Logger LOG = Logger.getLogger(AmOps.class);
    private AsyncAm asyncAm;
    private Counters counters;

    public AmOps(AsyncAm asyncAm, Counters counters) {
        this.asyncAm = asyncAm;
        this.counters = counters;
    }

    @Override
    public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException {
        try {
            counters.increment(Counters.Key.AM_statBlob);
            BlobDescriptor blobDescriptor = unwindExceptions(() -> asyncAm.statBlob(domain, volumeName, blobName).get());
            LOG.debug("AM.readMetadata, volume=" + volumeName + ", blobName=" + blobName + ", fieldCount=" + blobDescriptor.metadata.size());
            if (blobDescriptor.metadata.size() == 0) {
                System.out.println("WTF");
            }
            return Optional.of(blobDescriptor.getMetadata());
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                return Optional.empty();
            }

            LOG.error("AM.statBlob() returned error code " + e.getErrorCode() + ", volume=" + volumeName + ", blobName=" + blobName, e);
            throw new IOException(e);
        } catch (Exception e) {
            LOG.error("AM.statBlob() failed, volume=" + volumeName + ", blobName=" + blobName, e);
            throw new IOException(e);
        }
    }

    @Override
    public void writeMetadata(String domain, String volume, String blobName, Map<String, String> metadata, boolean deferrable) throws IOException {
        try {
            unwindExceptions(() -> {
                counters.increment(Counters.Key.AM_updateMetadataTx);
                TxDescriptor tx = asyncAm.startBlobTx(domain, volume, blobName, 0).get();
                asyncAm.updateMetadata(domain, volume, blobName, tx, metadata).get();
                asyncAm.commitBlobTx(domain, volume, blobName, tx).get();
                LOG.debug("AM.writeMetadata, volume=" + volume + ", blobName=" + blobName + ", fieldCount=" + metadata.size());
                return null;
            });
        } catch (Exception e) {
            LOG.error("AM.writeMetadata() failed, volume=" + volume + ", blobName=" + blobName, e);
            throw new IOException(e);
        }
    }

    @Override
    public ByteBuffer readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int objectSize) throws IOException {
        try {
            return unwindExceptions(() -> {
                counters.increment(Counters.Key.AM_getBlob);
                ByteBuffer byteBuffer = asyncAm.getBlob(domain, volumeName, blobName, objectSize, objectOffset).get();
                LOG.debug("AM.readObject, volume=" + volumeName + ", blobName=" + blobName + ", buf=" + byteBuffer.remaining());
                if (byteBuffer.remaining() == 0) {
                    byteBuffer = ByteBuffer.allocate(objectSize);
                }
                return byteBuffer;
            });
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                throw new FileNotFoundException();
            } else {
                LOG.error("am.getBlob() failed, volume=" + volumeName + ", blobName=" + blobName, e);
                throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("am.getBlob() failed, volume=" + volumeName + ", blobName=" + blobName, e);
            throw new IOException(e);
        }
    }

    @Override
    public void writeObject(String domain, String volume, String blobName, ObjectOffset objectOffset, ByteBuffer buf, int objectSize, boolean deferrable) throws IOException {
        try {
            unwindExceptions(() -> {
                counters.increment(Counters.Key.AM_updateBlobTx);
                TxDescriptor tx = asyncAm.startBlobTx(domain, volume, blobName, 0).get();
                ByteBuffer dupe = buf.duplicate();
                int length = dupe.remaining();
                asyncAm.updateBlob(domain, volume, blobName, tx, dupe, dupe.remaining(), objectOffset, false).get();
                asyncAm.commitBlobTx(domain, volume, blobName, tx).get();
                LOG.debug("AM.writeObject, volume=" + volume + ", blobName=" + blobName + ", buf=" + length);
                return null;
            });
        } catch (ApiException e) {
            LOG.error("AM.updateBlob() TX returned error code " + e.getErrorCode() + ", volume=" + volume + ", blobName=" + blobName + ", objectOffset=" + objectOffset, e);
            throw new IOException(e);
        } catch (IOException e) {
            LOG.error("AM.updateBlob() TX got an IOException, volume=" + volume + ", blobName=" + blobName + ", objectOffset=" + objectOffset, e);
            throw e;
        } catch (Exception e) {
            LOG.error("AM.updateBlob() TX got an Exception, volume=" + volume + ", blobName=" + blobName + ", objectOffset=" + objectOffset, e);
            throw new IOException(e);
        }
    }

    @Override
    public List<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException {
        try {
            return unwindExceptions(() -> {
                counters.increment(Counters.Key.AM_volumeContents);
                List<BlobMetadata> result = asyncAm.volumeContents(domain, volume, Integer.MAX_VALUE, 0, blobNamePrefix, PatternSemantics.PREFIX, null, BlobListOrder.UNSPECIFIED, false).get()
                        .getBlobs()
                        .stream()
                        .map(bd -> new BlobMetadata(bd.getName(), bd.getMetadata()))
                        .collect(Collectors.toList());
                LOG.debug("AM.volumeContents, volume=" + volume + ", count=" + result.size());
                return result;
            });
        } catch (Exception e) {
            LOG.error("AM.volumeContents() TX got an Exception, volume=" + volume + ", prefix=" + blobNamePrefix, e);
            throw new IOException(e);
        }
    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        try {
            unwindExceptions(() -> asyncAm.deleteBlob(domain, volume, blobName).get());
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                return;
            }
            LOG.error("AM.deleteBlob() returned error code " + e.getErrorCode() + ", volume=" + volume + ", blobName=" + blobName, e);
            throw new IOException(e);
        } catch (Exception e) {
            LOG.error("AM.deleteBlob() failed, volume=" + volume + ", blobName=" + blobName, e);
            throw new IOException(e);
        }
    }

    @Override
    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException {
        try {
            unwindExceptions(() -> asyncAm.renameBlob(domain, volumeName, oldName, newName));
            LOG.debug("AM.renameBlob, volume=" + volumeName + ", oldName=" + oldName + ", newName=" + newName);
        } catch (Exception e) {
            LOG.error("AM.renameBlob() failed, volume=" + volumeName + ", blobName=" + oldName, e);
            throw new IOException(e);
        }
    }
}

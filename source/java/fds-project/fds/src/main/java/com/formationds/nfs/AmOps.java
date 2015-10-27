package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.PatternSemantics;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.TimeoutException;
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
        String description = "volume=" + volumeName + ", blobName=" + blobName;

        try {
            counters.increment(Counters.Key.AM_statBlob);
            BlobDescriptor blobDescriptor = unwindExceptions(() -> asyncAm.statBlob(domain, volumeName, blobName).get());
            LOG.debug("AM.readMetadata, " + description + ", fieldCount=" + blobDescriptor.getMetadata().size());
            return Optional.of(blobDescriptor.getMetadata());
        } catch (ApiException e) {
            switch (e.getErrorCode()) {
                case MISSING_RESOURCE:
                    return Optional.empty();

                case TIMEOUT:
                    LOG.warn("AM.readMetadata timed out, " + description);
                    throw new TimeoutException();

                default:
                    LOG.error("AM.readMetadata returned error code " + e.getErrorCode() + ", " + description, e);
                    throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("AM.readMetadata failed, " + description, e);
            throw new IOException(e);
        }
    }

    @Override
    public void writeMetadata(String domain, String volume, String blobName, Map<String, String> metadata, boolean deferrable) throws IOException {
        String description = "volume=" + volume + ", blobName=" + blobName + ", fieldCount=" + metadata.size();
        try {
            unwindExceptions(() -> {
                counters.increment(Counters.Key.AM_updateMetadataTx);
                TxDescriptor tx = asyncAm.startBlobTx(domain, volume, blobName, 0).get();
                asyncAm.updateMetadata(domain, volume, blobName, tx, metadata).get();
                asyncAm.commitBlobTx(domain, volume, blobName, tx).get();
                LOG.debug("AM.writeMetadata, " + description);
                return null;
            });
        } catch (ApiException e) {
            switch (e.getErrorCode()) {
                case TIMEOUT:
                    LOG.warn("AM.writeMetadata timed out, " + description);
                    throw new TimeoutException();

                default:
                    LOG.error("AM.writeMetadata returned error code " + e.getErrorCode() + ", " + description, e);
                    throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("AM.writeMetadata() failed, " + description, e);
            throw new IOException(e);
        }
    }

    @Override
    public ByteBuffer readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int objectSize) throws IOException {
        String description = "volume=" + volumeName + ", blobName=" + blobName + ", objectOffset=" + objectOffset.getValue();
        try {
            return unwindExceptions(() -> {
                counters.increment(Counters.Key.AM_getBlob);
                ByteBuffer byteBuffer = asyncAm.getBlob(domain, volumeName, blobName, objectSize, objectOffset).get();
                LOG.debug("AM.readObject, " + description + ", buf=" + byteBuffer.remaining());
                if (byteBuffer.remaining() == 0) {
                    byteBuffer = ByteBuffer.allocate(objectSize);
                }
                return byteBuffer;
            });
        } catch (ApiException e) {
            switch (e.getErrorCode()) {
                case MISSING_RESOURCE:
                    throw new FileNotFoundException();

                case TIMEOUT:
                    LOG.warn("am.readObject timed out, " + description);
                    throw new TimeoutException();

                default:
                    LOG.error("am.readObject failed, " + description, e);
                    throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("am.readObject failed, " + description, e);
            throw new IOException(e);
        }
    }

    @Override
    public void writeObject(String domain, String volume, String blobName, ObjectOffset objectOffset, ByteBuffer buf, int objectSize, boolean deferrable) throws IOException {
        ByteBuffer dupe = buf.duplicate();
        int length = dupe.remaining();
        String description = "volume=" + volume + ", blobName=" + blobName + "objectOffset=" + objectOffset.getValue() + ", buf" + length;

        try {
            unwindExceptions(() -> {
                counters.increment(Counters.Key.AM_updateBlobTx);
                TxDescriptor tx = asyncAm.startBlobTx(domain, volume, blobName, 0).get();
                asyncAm.updateBlob(domain, volume, blobName, tx, dupe, dupe.remaining(), objectOffset, false).get();
                asyncAm.commitBlobTx(domain, volume, blobName, tx).get();
                LOG.debug("AM.writeObject, " + description);
                return null;
            });
        } catch (ApiException e) {
            switch (e.getErrorCode()) {
                case TIMEOUT:
                    LOG.warn("AM.writeObject timed out, " + description);
                    throw new TimeoutException();

                default:
                    LOG.error("AM.writeObject got an APIException, " + description, e);
                    throw new IOException(e);
            }
        } catch (IOException e) {
            LOG.error("AM.writeObject got an IOException, " + description, e);
            throw e;
        } catch (Exception e) {
            LOG.error("AM.writeObject got an Exception, " + description, e);
            throw new IOException(e);
        }
    }

    @Override
    public List<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException {
        String description = "volume=" + volume + ", prefix=" + blobNamePrefix;
        try {
            return unwindExceptions(() -> {
                counters.increment(Counters.Key.AM_volumeContents);
                List<BlobMetadata> result = asyncAm.volumeContents(domain, volume, Integer.MAX_VALUE, 0, blobNamePrefix, PatternSemantics.PREFIX, null, BlobListOrder.UNSPECIFIED, false).get()
                        .getBlobs()
                        .stream()
                        .map(bd -> new BlobMetadata(bd.getName(), bd.getMetadata()))
                        .collect(Collectors.toList());
                LOG.debug("AM.volumeContents, " + description + ", count=" + result.size());
                return result;
            });
        } catch (ApiException e) {
            switch (e.getErrorCode()) {
                case TIMEOUT:
                    LOG.warn("AM.volumeContents timed out, " + description);
                    throw new TimeoutException();

                default:
                    LOG.error("AM.volumeContents got an APIException, " + description, e);
                    throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("AM.volumeContents got an Exception, " + description, e);
            throw new IOException(e);
        }
    }

    @Override
    public void flush() throws IOException {

    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        String description = "volume=" + volume + ", blobName=" + blobName;

        try {
            unwindExceptions(() -> asyncAm.deleteBlob(domain, volume, blobName).get());
            LOG.debug("AM.deleteBlob, " + description);
        } catch (ApiException e) {
            switch (e.getErrorCode()) {
                case MISSING_RESOURCE:
                    return;

                case TIMEOUT:
                    LOG.warn("AM.deleteBlob timed out, " + description);
                    throw new TimeoutException();

                default:
                    LOG.error("AM.deleteBlob got an APIException, " + description, e);
                    throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("AM.deleteBlob failed, " + description, e);
            throw new IOException(e);
        }
    }

    @Override
    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException {
        String description = "volume=" + volumeName + ", oldName=" + oldName + ", newName=" + newName;
        try {
            unwindExceptions(() -> asyncAm.renameBlob(domain, volumeName, oldName, newName));
            LOG.debug("AM.renameBlob, " + description);
        } catch (ApiException e) {
            switch (e.getErrorCode()) {
                case TIMEOUT:
                    LOG.warn("AM.renameBlob timed out, " + description);
                    throw new TimeoutException();

                default:
                    LOG.error("AM.renameBlob got an APIException, " + description, e);
                    throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("AM.renameBlob failed, " + description, e);
            throw new IOException(e);
        }
    }
}

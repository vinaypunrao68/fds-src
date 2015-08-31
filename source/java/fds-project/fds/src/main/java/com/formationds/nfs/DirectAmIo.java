package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.protocol.*;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.BlobWithMetadata;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.*;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class DirectAmIo implements Io {
    private static final Logger LOG = Logger.getLogger(DirectAmIo.class);
    private AsyncAm asyncAm;

    public DirectAmIo(AsyncAm asyncAm) {
        this.asyncAm = asyncAm;
    }

    @Override
    public <T> T mapMetadata(String domain, String volumeName, String blobName, MetadataMapper<T> mapper) throws IOException {
        try {
            BlobDescriptor blobDescriptor = unwindExceptions(() -> asyncAm.statBlob(domain, volumeName, blobName).get());
            return mapper.map(Optional.of(blobDescriptor.getMetadata()));
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                return mapper.map(Optional.empty());
            }

            LOG.error("AM.statBlob() returned error code " + e.getErrorCode() + ", volume=" + volumeName + ", blobName=" + blobName, e);
            throw new IOException(e);
        } catch (Exception e) {
            LOG.error("AM.statBlob() failed, volume=" + volumeName + ", blobName=" + blobName, e);
            throw new IOException(e);
        }
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, MetadataMutator mutator) throws IOException {
        mapMetadata(domain, volume, blobName, (om) -> {
            Map<String, String> meta = om.orElse(new HashMap<>());
            mutator.mutate(meta);
            try {
                unwindExceptions(() -> {
                    TxDescriptor tx = asyncAm.startBlobTx(domain, volume, blobName, 0).get();
                    asyncAm.updateMetadata(domain, volume, blobName, tx, meta).get();
                    asyncAm.commitBlobTx(domain, volume, blobName, tx).get();
                    return null;
                });
            } catch (Exception e) {
                LOG.error("AM.updateMetadata() failed, volume=" + volume + ", blobName=" + blobName, e);
                throw new IOException(e);
            }
            return null;
        });
    }

    @Override
    public void setMetadataOnEmptyBlob(String domain, String volume, String blobName, Map<String, String> map) throws IOException {
        try {
            unwindExceptions(() -> {
                asyncAm.updateBlobOnce(domain, volume, blobName, 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), map).get();
                return null;
            });
        } catch (Exception e) {
            LOG.error("AM.updateMetadata() failed, volume=" + volume + ", blobName=" + blobName, e);
            throw new IOException(e);
        }

    }

    // Objects will be either non-existent or have 'objectSize' bytes
    @Override
    public <T> T mapObject(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMapper<T> objectMapper) throws IOException {
        Optional<BlobWithMetadata> blobWithMeta = null;
        try {
            blobWithMeta = Optional.of(unwindExceptions(() -> asyncAm.getBlobWithMeta(BlockyVfs.DOMAIN, volume, blobName, objectSize, objectOffset).get()));
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                blobWithMeta = Optional.empty();
            } else {
                LOG.error("AM.getBlobWithMeta() returned error code " + e.getErrorCode() + ", volume=" + volume + ", blobName=" + blobName, e);
                throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("AM.getBlobWithMeta() failed, volume=" + volume + ", blobName=" + blobName, e);
            throw new IOException(e);
        }

        Optional<ObjectView> view = blobWithMeta.map(bwm -> {
            if (bwm.getBytes().remaining() == 0) {
                return new ObjectView(bwm.getBlobDescriptor().getMetadata(), ByteBuffer.allocate(objectSize));
            } else {
                return new ObjectView(bwm.getBlobDescriptor().getMetadata(), bwm.getBytes().slice());
            }
        });

        try {
            return objectMapper.map(view);
        } catch (Exception e) {
            throw new IOException(e);
        }
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMutator mutator) throws IOException {
        mapObject(domain, volume, blobName, objectSize, objectOffset, (x) -> {
            ObjectView ov = x.orElseGet(() -> new ObjectView(new HashMap<>(), ByteBuffer.allocate(objectSize)));
            mutator.mutate(ov);
            ByteBuffer buf = ov.getBuf();
            try {
                unwindExceptions(() -> {
                    return asyncAm.updateBlobOnce(domain,
                            volume,
                            blobName,
                            0,
                            buf,
                            objectSize,
                            objectOffset,
                            ov.getMetadata()).get();
                });
            } catch (Exception e) {
                LOG.error("AM.updateBlobOnce() failed, volume=" + volume + ", blobName=" + blobName, e);
                throw new IOException(e);
            }
            return null;
        });
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, Map<String, String> metadata) throws IOException {
        try {
            unwindExceptions(() -> {
                return asyncAm.updateBlobOnce(domain,
                        volume,
                        blobName,
                        0,
                        byteBuffer,
                        objectSize,
                        objectOffset,
                        metadata).get();
            });
        } catch (Exception e) {
            LOG.error("AM.updateBlobOnce() failed, volume=" + volume + ", blobName=" + blobName, e);
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
    public <T> List<T> scan(String domain, String volume, String blobNamePrefix, MetadataMapper<T> mapper) throws IOException {
        try {
            long then = System.currentTimeMillis();
            List<BlobDescriptor> blobDescriptors = unwindExceptions(() -> asyncAm.volumeContents(domain, volume, Integer.MAX_VALUE, 0, blobNamePrefix, PatternSemantics.PREFIX, BlobListOrder.UNSPECIFIED, false).get());
            long elapsed = System.currentTimeMillis() - then;
            LOG.debug("AM.volumeContents() returned " + blobDescriptors.size() + " entries in " + elapsed + " ms");
            List<T> result = new ArrayList<>(blobDescriptors.size());
            for (BlobDescriptor bd : blobDescriptors) {
                result.add(mapper.map(Optional.of(bd.getMetadata())));
            }
            return result;
        } catch (Exception e) {
            LOG.error("AM.volumeContents() failed, volume=" + volume + ", keyPrefix=" + blobNamePrefix, e);
            throw new IOException(e);
        }
    }
}

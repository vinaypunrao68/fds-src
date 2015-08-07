package com.formationds.nfs;

import com.formationds.apis.TxDescriptor;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import org.apache.log4j.Logger;
import org.dcache.nfs.status.NoEntException;
import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;

import javax.security.auth.Subject;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Optional;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class InodeMap {
    private static final Logger LOG = Logger.getLogger(InodeMap.class);
    private final AsyncAm asyncAm;
    private final ExportResolver exportResolver;
    public static final Inode ROOT;
    public static final InodeMetadata ROOT_METADATA;
    private final Cache<CacheKey, InodeMetadata> cache;

    static {
        byte[] bytes = new byte[8];
        ByteBuffer bb = ByteBuffer.wrap(bytes);
        bb.putLong(Long.MAX_VALUE);
        ROOT = new Inode(new FileHandle(0, Integer.MAX_VALUE, Stat.Type.DIRECTORY.toMode(), bytes));
        ROOT_METADATA = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0755, Long.MAX_VALUE, Integer.MAX_VALUE);
    }

    private final Chunker chunker;

    public InodeMap(AsyncAm asyncAm, ExportResolver exportResolver) {
        this.asyncAm = asyncAm;
        this.exportResolver = exportResolver;
        cache = CacheBuilder.newBuilder()
                .maximumSize(100)
                .build();
        chunker = new Chunker(new ChunkyAm(asyncAm));
    }


    public Optional<InodeMetadata> stat(Inode inode) throws IOException {
        if (isRoot(inode)) {
            return Optional.of(ROOT_METADATA);
        }

        String blobName = blobName(inode);
        String volumeName = volumeName(inode);

        CacheKey key = new CacheKey(volumeName, blobName);
        try {
            InodeMetadata result = cache.getIfPresent(key);
            if (result != null) {
                return Optional.of(result);
            }
            BlobDescriptor blobDescriptor = unwindExceptions(() -> asyncAm.statBlob(BlockyVfs.DOMAIN, volumeName, blobName).get());
            InodeMetadata metadata = new InodeMetadata(blobDescriptor);
            cache.put(key, metadata);
            return Optional.of(metadata);
        } catch (ApiException e) {
            cache.invalidate(key);
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                return Optional.empty();
            } else {
                throw new IOException(e);
            }
        } catch (Exception e) {
            cache.invalidate(key);
            throw new IOException(e);
        }
    }

    public static String blobName(Inode inode) {
        return "inode-" + InodeMetadata.fileId(inode);
    }

    public InodeMetadata write(Inode inode, byte[] data, long offset, int count) throws IOException {
        Optional<InodeMetadata> stat = stat(inode);
        if (!stat.isPresent()) {
            throw new NoEntException();
        }

        String volumeName = exportResolver.volumeName(inode.exportIndex());
        String blobName = InodeMap.blobName(inode);
        CacheKey cacheKey = new CacheKey(volumeName, blobName);

        try {
            long byteCount = stat.get().getSize();
            int length = Math.min(data.length, count);
            byteCount = Math.max(byteCount, offset + length);
            InodeMetadata im = stat.get()
                    .withUpdatedAtime()
                    .withUpdatedMtime()
                    .withUpdatedCtime()
                    .withUpdatedSize(byteCount);
            int objectSize = exportResolver.objectSize(volumeName);
            chunker.write(BlockyVfs.DOMAIN, volumeName, blobName, objectSize, data, offset, count, byteCount, im.asMap());
            cache.put(cacheKey, im);
            return im;
        } catch (Exception e) {
            String message = "Error writing " + volumeName + "." + blobName;
            LOG.error(message, e);
            cache.invalidate(cacheKey);
            throw new IOException(message, e);
        }
    }
    public Inode create(InodeMetadata metadata) throws IOException {
        return doUpdate(metadata, 1);
    }

    private Inode doUpdate(InodeMetadata metadata, int mode) throws IOException {
        String volume = exportResolver.volumeName((int) metadata.getVolumeId());
        String blobName = blobName(metadata.asInode());
        CacheKey key = new CacheKey(volume, blobName);
        try {
            TxDescriptor desc = unwindExceptions(() -> {
                TxDescriptor tx = asyncAm.startBlobTx(BlockyVfs.DOMAIN, volume, blobName, mode).get();
                asyncAm.updateMetadata(BlockyVfs.DOMAIN, volume, blobName, tx, metadata.asMap()).get();
                asyncAm.commitBlobTx(BlockyVfs.DOMAIN, volume, blobName, tx).get();
                return tx;
            });
            cache.put(key, metadata);
        } catch (Exception e) {
            cache.invalidate(key);
            LOG.error("error creating " + blobName + " in volume " + volume, e);
            throw new IOException(e);
        }
        return metadata.asInode();
    }

    public int volumeId(Inode inode) {
        return inode.exportIndex();
    }

    public String volumeName(Inode inode) {
        return exportResolver.volumeName(inode.exportIndex());
    }

    public long fileId(Inode inode) {
        return InodeMetadata.fileId(inode);
    }

    public void update(InodeMetadata... entries) throws IOException {
        for (InodeMetadata entry : entries) {
            doUpdate(entry, 0);
        }
    }

    public void remove(Inode inode) throws IOException {
        String blobName = blobName(inode);
        String volumeName = volumeName(inode);
        CacheKey key = new CacheKey(volumeName, blobName);

        try {
            unwindExceptions(() -> asyncAm.deleteBlob(BlockyVfs.DOMAIN, volumeName, blobName).get());
        } catch (Exception e) {
            LOG.debug("error creating " + blobName + " in volume " + volumeName, e);
            throw new IOException(e);
        } finally {
            cache.invalidate(key);
        }
    }

    public static boolean isRoot(Inode inode) {
        return InodeMetadata.fileId(inode) == Long.MAX_VALUE;
    }

    private static class CacheKey {
        private String volume;
        private String blobName;

        public CacheKey(String volume, String blobName) {
            this.volume = volume;
            this.blobName = blobName;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            CacheKey cacheKey = (CacheKey) o;

            if (!blobName.equals(cacheKey.blobName)) return false;
            if (!volume.equals(cacheKey.volume)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = volume.hashCode();
            result = 31 * result + blobName.hashCode();
            return result;
        }
    }
}

package com.formationds.nfs;

import com.formationds.hadoop.FdsInputStream;
import com.formationds.hadoop.FdsOutputStream;
import com.formationds.hadoop.OwnerGroupInfo;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import org.apache.commons.io.IOUtils;
import org.apache.log4j.Logger;
import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.Inode;
import org.json.JSONObject;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.ExecutionException;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class SimpleInodeIndex implements InodeIndex {
    private static final Logger LOG = Logger.getLogger(SimpleInodeIndex.class);
    private AsyncAm asyncAm;
    private ExportResolver exportResolver;
    private Cache<CacheKey, JSONObject> cache;

    public SimpleInodeIndex(AsyncAm asyncAm, ExportResolver exportResolver) {
        this.asyncAm = asyncAm;
        this.exportResolver = exportResolver;
        cache = CacheBuilder.newBuilder()
                .maximumSize(100)
                .build();
    }

    @Override
    public Optional<InodeMetadata> lookup(Inode parent, String name) throws IOException {
        JSONObject indexEntry = loadIndexEntry(parent);
        if (!indexEntry.has(name)) {
            return Optional.empty();
        }
        return Optional.of(new InodeMetadata(indexEntry.getJSONObject(name)));
    }

    @Override
    public List<DirectoryEntry> list(InodeMetadata directory) throws IOException {
        JSONObject indexEntry = loadIndexEntry(directory.asInode());
        List<DirectoryEntry> results = new ArrayList<>(indexEntry.keySet().size());
        Iterator<String> keys = indexEntry.keys();
        while (keys.hasNext()) {
            String name = keys.next();
            InodeMetadata value = new InodeMetadata(indexEntry.getJSONObject(name));
            results.add(value.asDirectoryEntry(directory.getFileId()));
        }
        return results;
    }

    @Override
    public void index(InodeMetadata... entries) throws IOException {
        for (InodeMetadata entry : entries) {
            String volumeName = exportResolver.volumeName((int) entry.getVolumeId());
            Map<Long, String> links = entry.getLinks();
            for (long parentId : links.keySet()) {
                String name = links.get(parentId);
                JSONObject indexEntry = loadIndexEntry(volumeName, parentId);
                indexEntry.put(name, entry.asJsonObject());
                writeIndexEntry(volumeName, parentId, indexEntry);
            }
        }
    }

    private void writeIndexEntry(String volumeName, long inodeId, JSONObject indexEntry) throws IOException {
        int objectSize = exportResolver.objectSize(volumeName);
        FdsOutputStream outputStream = FdsOutputStream.openNew(asyncAm, BlockyVfs.DOMAIN, volumeName, blobName(inodeId), objectSize, new OwnerGroupInfo("foo", "bar"));
        IOUtils.write(indexEntry.toString(), outputStream);
        outputStream.flush();
        outputStream.close();
    }

    @Override
    public void remove(InodeMetadata inodeMetadata) throws IOException {
        String volume = exportResolver.volumeName((int) inodeMetadata.getVolumeId());
        long fileId = inodeMetadata.getFileId();
        String blobName = blobName(fileId);
        try {
            unwindExceptions(() -> asyncAm.deleteBlob(BlockyVfs.DOMAIN, volume, blobName).get());
        } catch (ApiException e) {
            if (!e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("Error removing inode index, volume=" + volume + ", blobName=" + blobName);
            throw new IOException(e);
        }
        cache.invalidate(new CacheKey(volume, fileId));
        cache.getIfPresent(new CacheKey(volume, fileId));
    }

    @Override
    public void unlink(Inode parentInode, String path) throws IOException {
        JSONObject indexEntry = loadIndexEntry(parentInode);
        String volumeName = exportResolver.volumeName(parentInode.exportIndex());
        if (indexEntry.has(path)) {
            indexEntry.remove(path);
            writeIndexEntry(volumeName, InodeMetadata.fileId(parentInode), indexEntry);
        }
    }


    private JSONObject loadIndexEntry(Inode inode) throws IOException {
        String volume = exportResolver.volumeName(inode.exportIndex());
        long fileId = InodeMetadata.fileId(inode);
        return loadIndexEntry(volume, fileId);
    }

    private JSONObject loadIndexEntry(String volume, long fileId) throws IOException {
        CacheKey cacheKey = new CacheKey(volume, fileId);
        try {
            return cache.get(cacheKey, () -> {
                int objectSize = exportResolver.objectSize(volume);
                String blobName = blobName(fileId);
                FdsInputStream in = new FdsInputStream(asyncAm, BlockyVfs.DOMAIN, volume, blobName, objectSize);
                String source = IOUtils.toString(in);
                if (source.isEmpty()) {
                    source = "{}";
                }
                return new JSONObject(source);
            });
        } catch (ExecutionException e) {
            LOG.error("Error fetching index entry, volume=" + volume + ", fileId=" + fileId, e);
            if (e.getCause() instanceof IOException) {
                throw (IOException) e.getCause();
            } else {
                throw new IOException(e.getCause());
            }
        }
    }

    private String blobName(long fileId) {
        return "index-" + fileId;
    }

    private static class CacheKey {
        String volume;
        long fileId;

        public CacheKey(String volume, long fileId) {
            this.volume = volume;
            this.fileId = fileId;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            CacheKey cacheKey = (CacheKey) o;

            if (fileId != cacheKey.fileId) return false;
            if (!volume.equals(cacheKey.volume)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = volume.hashCode();
            result = 31 * result + (int) (fileId ^ (fileId >>> 32));
            return result;
        }
    }
}

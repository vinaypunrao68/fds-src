package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.ExecutionException;

public class TransactionalIo {
    private IoOps io;
    private Cache<MetaKey, Object> metaLocks;
    private Cache<ObjectKey, Object> objectLocks;

    public TransactionalIo(IoOps io) {
        this.io = io;
        objectLocks = CacheBuilder.newBuilder().maximumSize(500).build();
        metaLocks = CacheBuilder.newBuilder().maximumSize(20000).build();
    }

    public <T> T mapMetadata(String domain, String volumeName, String blobName, MetadataMapper<T> mapper) throws IOException {
        MetaKey key = new MetaKey(domain, volumeName, blobName);
        synchronized (metaLock(key)) {
            return mapper.map(blobName, io.readMetadata(domain, volumeName, blobName));
        }
    }

    public <T> T mutateMetadata(String domain, String volume, String blobName, boolean deferrable, MetadataMutator<T> mutator) throws IOException {
        MetaKey key = new MetaKey(domain, volume, blobName);

        T result;
        synchronized (metaLock(key)) {
            Map<String, String> map = io.readMetadata(domain, volume, blobName)
                    .orElse(new HashMap<>());

            result = mutator.mutate(map);
            io.writeMetadata(domain, volume, blobName, map, deferrable);
        }
        return result;
    }

    public void mutateMetadata(String domain, String volume, String blobName, Map<String, String> map, boolean deferrable) throws IOException {
        MetaKey key = new MetaKey(domain, volume, blobName);

        synchronized (metaLock(key)) {
            io.writeMetadata(domain, volume, blobName, map, deferrable);
        }
    }

    public <T> T mapObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMapper<T> objectMapper) throws IOException {
        MetaKey metaKey = new MetaKey(domain, volume, blobName);
        ObjectKey objectKey = new ObjectKey(domain, volume, blobName, objectOffset);

        synchronized (objectLock(objectKey)) {
            Optional<Map<String, String>> metadata;

            synchronized (metaLock(metaKey)) {
                metadata = io.readMetadata(domain, volume, blobName);
            }

            if (!metadata.isPresent()) {
                return objectMapper.map(Optional.empty());
            }

            ByteBuffer buf = io.readCompleteObject(domain, volume, blobName, objectOffset, objectSize);
            return objectMapper.map(Optional.of(new ObjectAndMetadata(metadata.get(), buf.duplicate())));
        }
    }

    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, boolean deferrable, ObjectMutator mutator) throws IOException {
        MetaKey metaKey = new MetaKey(domain, volume, blobName);
        ObjectKey objectKey = new ObjectKey(domain, volume, blobName, objectOffset);

        synchronized (objectLock(objectKey)) {
            ByteBuffer buffer = null;
            try {
                buffer = io.readCompleteObject(domain, volume, blobName, objectOffset, objectSize);
            } catch (FileNotFoundException e) {
                buffer = ByteBuffer.allocate(objectSize);
            }
            Map<String, String> metadata;
            synchronized (metaLock(metaKey)) {
                metadata = io.readMetadata(domain, volume, blobName).orElse(new HashMap<>());
                ObjectAndMetadata om = new ObjectAndMetadata(metadata, buffer.duplicate());
                mutator.mutate(om);
                io.writeMetadata(domain, volume, blobName, metadata, deferrable);
            }
            io.writeObject(domain, volume, blobName, objectOffset, buffer, objectSize, deferrable);
        }
    }

    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        MetaKey metaKey = new MetaKey(domain, volume, blobName);

        synchronized (metaLock(metaKey)) {
            io.deleteBlob(domain, volume, blobName);
        }
    }

    public void renameBlob(String domain, String volume, String oldName, String newName) throws IOException {
        MetaKey metaKey = new MetaKey(domain, volume, oldName);

        synchronized (metaLock(metaKey)) {
            io.renameBlob(domain, volume, oldName, newName);
        }
    }

    public <T> List<T> scan(String domain, String volume, String blobNamePrefix, MetadataMapper<T> mapper) throws IOException {
        Collection<BlobMetadata> mds = io.scan(domain, volume, blobNamePrefix);
        List<T> result = new ArrayList<>(mds.size());
        for (BlobMetadata meta : mds) {
            result.add(mapper.map(meta.getBlobName(), Optional.of(meta.getMetadata())));
        }
        return result;
    }

    public void flush() throws IOException {
        io.flush();
    }

    private Object metaLock(MetaKey key) {
        try {
            return metaLocks.get(key, () -> new Object());
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    private Object objectLock(ObjectKey key) {
        try {
            return objectLocks.get(key, () -> new Object());
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }
}

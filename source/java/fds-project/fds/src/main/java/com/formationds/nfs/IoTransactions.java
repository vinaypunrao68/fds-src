package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.ExecutionException;

public class IoTransactions implements TransactionalIo {
    private IoOps io;
    private Counters counters;
    private Cache<MetaKey, Object> metaLocks;
    private Cache<ObjectKey, Object> objectLocks;

    public IoTransactions(IoOps io, Counters counters) {
        this.io = io;
        this.counters = counters;
        objectLocks = CacheBuilder.newBuilder().maximumSize(500).build();
        metaLocks = CacheBuilder.newBuilder().maximumSize(20000).build();
    }

    @Override
    public <T> T mapMetadata(String domain, String volumeName, String blobName, MetadataMapper<T> mapper) throws IOException {
        MetaKey key = new MetaKey(domain, volumeName, blobName);
        synchronized (metaLock(key)) {
            return mapper.map(io.readMetadata(domain, volumeName, blobName));
        }
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, MetadataMutator mutator) throws IOException {
        MetaKey key = new MetaKey(domain, volume, blobName);

        synchronized (metaLock(key)) {
            Map<String, String> map = io.readMetadata(domain, volume, blobName)
                    .orElse(new HashMap<>());

            mutator.mutate(map);
            io.writeMetadata(domain, volume, blobName, map, true);
        }
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, Map<String, String> map, boolean deferrable) throws IOException {
        MetaKey key = new MetaKey(domain, volume, blobName);

        synchronized (metaLock(key)) {
            io.writeMetadata(domain, volume, blobName, map, deferrable);
        }
    }

    @Override
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

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMutator mutator) throws IOException {
        MetaKey metaKey = new MetaKey(domain, volume, blobName);
        ObjectKey objectKey = new ObjectKey(domain, volume, blobName, objectOffset);

        synchronized (objectLock(objectKey)) {
            ByteBuffer buffer = io.readCompleteObject(domain, volume, blobName, objectOffset, objectSize);
            Map<String, String> metadata;
            synchronized (metaLock(metaKey)) {
                metadata = io.readMetadata(domain, volume, blobName).orElse(new HashMap<>());
                ObjectAndMetadata om = new ObjectAndMetadata(metadata, buffer.duplicate());
                mutator.mutate(om);
                io.writeMetadata(domain, volume, blobName, metadata, true);
            }
            io.writeObject(domain, volume, blobName, objectOffset, buffer, objectSize);
        }
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, Map<String, String> metadata) throws IOException {
        MetaKey metaKey = new MetaKey(domain, volume, blobName);
        ObjectKey objectKey = new ObjectKey(domain, volume, blobName, objectOffset);

        synchronized (objectLock(objectKey)) {
            io.writeObject(domain, volume, blobName, objectOffset, byteBuffer, objectSize);
            synchronized (metaLock(metaKey)) {
                io.writeMetadata(domain, volume, blobName, metadata, true);
            }
        }
    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        MetaKey metaKey = new MetaKey(domain, volume, blobName);

        synchronized (metaLock(metaKey)) {
            io.deleteBlob(domain, volume, blobName);
        }
    }

    @Override
    public <T> List<T> scan(String domain, String volume, String blobNamePrefix, MetadataMapper<T> mapper) throws IOException {
        List<Map<String, String>> mds = io.scan(domain, volume, blobNamePrefix);
        List<T> result = new ArrayList<>(mds.size());
        for (Map<String, String> meta : mds) {
            result.add(mapper.map(Optional.of(meta)));
        }
        return result;
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

package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ExecutionException;

public class IoCache implements Io {
    private Io io;
    private Counters counters;
    private Cache<MetadataCacheKey, Optional<Map<String, String>>> metadataCache;
    private Cache<ObjectCacheKey, ByteBuffer> objectCache;

    private Cache<MetadataCacheKey, Object> metadataCacheLocks;
    private Cache<ObjectCacheKey, Object> objectCacheLocks;

    public IoCache(Io io, Counters counters) {
        this.io = io;
        this.counters = counters;
        metadataCache = CacheBuilder.newBuilder().maximumSize(20000).build();
        objectCache = CacheBuilder.newBuilder().maximumSize(500).build();
        objectCacheLocks = CacheBuilder.newBuilder().maximumSize(20000).build();
        metadataCacheLocks = CacheBuilder.newBuilder().maximumSize(20000).build();
    }

    @Override
    public <T> T mapMetadata(String domain, String volumeName, String blobName, MetadataMapper<T> mapper) throws IOException {
        MetadataCacheKey key = new MetadataCacheKey(domain, volumeName, blobName);
        synchronized (metadataLock(key)) {
            Optional<Map<String, String>> value = metadataCache.getIfPresent(key);
            if (value == null) {
                value = io.mapMetadata(domain, volumeName, blobName, om -> om);
                metadataCache.put(key, value);
                counters.increment(Counters.Key.metadataCacheMiss);
            } else {
                counters.increment(Counters.Key.metadataCacheHit);
            }

            // Map a copy
            return mapper.map(value.map(x -> new HashMap<>(x)));
        }
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, MetadataMutator mutator) throws IOException {
        MetadataCacheKey key = new MetadataCacheKey(domain, volume, blobName);
        Map<String, String> updatedValue = new HashMap<>();

        synchronized (metadataLock(key)) {
            Optional<Map<String, String>> oldValue = metadataCache.getIfPresent(key);
            if (oldValue == null) {
                oldValue = io.mapMetadata(domain, volume, blobName, om -> om);
                counters.increment(Counters.Key.metadataCacheMiss);
            } else {
                counters.increment(Counters.Key.metadataCacheHit);
            }
            if (oldValue.isPresent()) {
                updatedValue.putAll(oldValue.get());
            }
            mutator.mutate(updatedValue);
            metadataCache.put(key, Optional.of(updatedValue));
        }

        io.mutateMetadata(domain, volume, blobName, metadata -> {
            metadata.clear();
            metadata.putAll(updatedValue);
        });
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, Map<String, String> map, boolean deferrable) throws IOException {
        metadataCache.put(new MetadataCacheKey(domain, volume, blobName), Optional.of(map));
        io.mutateMetadata(domain, volume, blobName, map, deferrable);
    }

    @Override
    public <T> T mapObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMapper<T> objectMapper) throws IOException {
        ObjectCacheKey key = new ObjectCacheKey(domain, volume, blobName, objectOffset);

        synchronized (objectLock(key)) {
            Optional<Map<String, String>> metadata = mapMetadata(domain, volume, blobName, om -> om);
            if (!metadata.isPresent()) {
                return objectMapper.map(Optional.empty());
            }

            ByteBuffer cacheEntry = objectCache.getIfPresent(key);
            if (cacheEntry == null) {
                cacheEntry = io.mapObjectAndMetadata(domain, volume, blobName, objectSize, objectOffset,
                        oov -> oov.get().getBuf().slice());
                objectCache.put(key, cacheEntry);
                counters.increment(Counters.Key.objectCacheMiss);
            } else {
                counters.increment(Counters.Key.objectCacheHit);
            }
            return objectMapper.map(Optional.of(new ObjectView(metadata.get(), cacheEntry.duplicate())));
        }
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMutator mutator) throws IOException {
        MetadataCacheKey metaKey = new MetadataCacheKey(domain, volume, blobName);
        ObjectCacheKey objectKey = new ObjectCacheKey(domain, volume, blobName, objectOffset);

        ObjectView[] objectView = new ObjectView[1];

        synchronized (objectLock(objectKey)) {
            ByteBuffer[] buf = new ByteBuffer[]{objectCache.getIfPresent(objectKey)};
            if (buf[0] == null) {
                counters.increment(Counters.Key.objectCacheMiss);
                buf[0] = io.mapObjectAndMetadata(domain, volume, blobName, objectSize, objectOffset,
                        oov -> {
                            if (oov.isPresent()) {
                                return oov.get().getBuf().slice();
                            } else {
                                return ByteBuffer.allocate(objectSize);
                            }
                        });
                objectCache.put(objectKey, buf[0]);
            } else {
                counters.increment(Counters.Key.objectCacheHit);
            }

            mapMetadata(domain, volume, blobName, om -> {
                if (!om.isPresent()) {
                    Map<String, String> metadata = new HashMap<>();
                    mutator.mutate(new ObjectView(metadata, buf[0].duplicate()));
                    metadataCache.put(metaKey, Optional.of(metadata));
                    objectView[0] = new ObjectView(new HashMap<>(metadata), buf[0].duplicate());
                } else {
                    ObjectView ov = new ObjectView(om.get(), buf[0].duplicate());
                    mutator.mutate(ov);
                    metadataCache.put(metaKey, Optional.of(ov.getMetadata()));
                    objectView[0] = new ObjectView(new HashMap<String, String>(om.get()), buf[0].duplicate());
                }
                return null;
            });
            io.mutateObjectAndMetadata(domain, volume, blobName, objectSize, objectOffset, objectView[0].getBuf(), new HashMap<>(objectView[0].getMetadata()));
        }
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, Map<String, String> metadata) throws IOException {
        MetadataCacheKey metaKey = new MetadataCacheKey(domain, volume, blobName);
        ObjectCacheKey objectKey = new ObjectCacheKey(domain, volume, blobName, objectOffset);

        synchronized (metadataLock(metaKey)) {
            metadataCache.put(metaKey, Optional.of(metadata));
            synchronized (objectLock(objectKey)) {
                objectCache.put(objectKey, byteBuffer.duplicate());
            }
        }

        io.mutateObjectAndMetadata(domain, volume, blobName, objectSize, objectOffset, byteBuffer, metadata);
    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        MetadataCacheKey metaKey = new MetadataCacheKey(domain, volume, blobName);
        ConcurrentMap<ObjectCacheKey, ByteBuffer> map = objectCache.asMap();

        synchronized (metadataLock(metaKey)) {
            metadataCache.invalidate(metaKey);
            Set<ObjectCacheKey> objectKeys = map.keySet();
            for (ObjectCacheKey objectKey : objectKeys) {
                if (objectKey.domain.equals(domain) &&
                        objectKey.volume.equals(volume) &&
                        objectKey.blob.equals(blobName)) {
                    map.remove(objectKey);
                }
            }
            metadataCache.cleanUp();
            objectCache.cleanUp();
        }

        io.deleteBlob(domain, volume, blobName);
    }

    @Override
    public <T> List<T> scan(String domain, String volume, String blobNamePrefix, MetadataMapper<T> mapper) throws IOException {
        return io.scan(domain, volume, blobNamePrefix, mapper);
    }

    private Object metadataLock(MetadataCacheKey key) {
        try {
            return metadataCacheLocks.get(key, () -> new Object());
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    private Object objectLock(ObjectCacheKey key) {
        try {
            return objectCacheLocks.get(key, () -> new Object());
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    private class ObjectCacheKey {
        private String domain;
        private String volume;
        private String blob;
        private long objectOffset;

        public ObjectCacheKey(String domain, String volume, String blob, ObjectOffset objectOffset) {
            this.domain = domain;
            this.volume = volume;
            this.blob = blob;
            this.objectOffset = objectOffset.getValue();
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            ObjectCacheKey that = (ObjectCacheKey) o;

            if (objectOffset != that.objectOffset) return false;
            if (!blob.equals(that.blob)) return false;
            if (!domain.equals(that.domain)) return false;
            if (!volume.equals(that.volume)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = domain.hashCode();
            result = 31 * result + volume.hashCode();
            result = 31 * result + blob.hashCode();
            result = 31 * result + (int) (objectOffset ^ (objectOffset >>> 32));
            return result;
        }
    }

    private class MetadataCacheKey {
        private String domain;
        private String volume;
        private String blob;

        public MetadataCacheKey(String domain, String volume, String blob) {
            this.domain = domain;
            this.volume = volume;
            this.blob = blob;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            MetadataCacheKey that = (MetadataCacheKey) o;

            if (!blob.equals(that.blob)) return false;
            if (!domain.equals(that.domain)) return false;
            if (!volume.equals(that.volume)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = domain.hashCode();
            result = 31 * result + volume.hashCode();
            result = 31 * result + blob.hashCode();
            return result;
        }
    }
}

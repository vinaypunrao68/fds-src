package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.nfs.deferred.CacheEntry;
import com.formationds.nfs.deferred.EvictingCache;
import com.formationds.util.IoConsumer;
import com.formationds.util.IoFunction;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;


public class DeferredIoOps implements IoOps {
    private static final Logger LOG = Logger.getLogger(DeferredIoOps.class);
    private IoOps io;
    private final EvictingCache<MetaKey, Map<String, String>> metadataCache;
    private final EvictingCache<ObjectKey, FdsObject> objectCache;
    private final List<IoConsumer<MetaKey>> metadataCommitListeners;

    private static class MetadataLoader implements EvictingCache.Loader<MetaKey, Map<String, String>> {
        private IoOps io;

        public MetadataLoader(IoOps io) {
            this.io = io;
        }

        @Override
        public CacheEntry<Map<String, String>> load(MetaKey key) throws IOException {
            Optional<Map<String, String>> om = io.readMetadata(key.domain, key.volume, key.blobName);
            if (om.isPresent()) {
                Map<String, String> metadata = om.get();
                return new CacheEntry<>(new ConcurrentHashMap<>(metadata), false, false);
            } else {
                return new CacheEntry<>(new ConcurrentHashMap<>(), false, true);
            }
        }
    }

    private static class ObjectLoader implements EvictingCache.Loader<ObjectKey, FdsObject> {
        private IoOps io;
        private IoFunction<String, Integer> maxObjectSizeProvider;

        public ObjectLoader(IoOps io, IoFunction<String, Integer> maxObjectSizeProvider) {
            this.io = io;
            this.maxObjectSizeProvider = maxObjectSizeProvider;
        }

        @Override
        public CacheEntry<FdsObject> load(ObjectKey objectKey) throws IOException {
            int maxObjectSize = maxObjectSizeProvider.apply(objectKey.volume);
            FdsObject o = io.readCompleteObject(objectKey.domain, objectKey.volume, objectKey.blobName, new ObjectOffset(objectKey.objectOffset), maxObjectSize);
            return new CacheEntry<>(o, false, false);
        }
    }

    public DeferredIoOps(IoOps io, IoFunction<String, Integer> maxObjectSizeForVolume) {
        this.io = io;
        metadataCache = new EvictingCache<>(
                new MetadataLoader(io),
                (key, cacheEntry) -> doCommitMetadata(key, cacheEntry),
                "Metadata cache",
                100000,
                1,
                TimeUnit.MINUTES);

        objectCache = new EvictingCache<>(
                new ObjectLoader(io, maxObjectSizeForVolume),
                (key, cacheEntry) -> doCommitObject(key, cacheEntry),
                "Object cache",
                500,
                1,
                TimeUnit.MINUTES);

        metadataCommitListeners = new CopyOnWriteArrayList<>();
    }

    public void addCommitListener(IoConsumer<MetaKey> listener) {
        metadataCommitListeners.add(listener);
    }

    public IoOps start() {
        metadataCache.start();
        objectCache.start();
        return this;
    }

    @Override
    public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException {
        MetaKey key = new MetaKey(domain, volumeName, blobName);
        CacheEntry<Map<String, String>> cacheEntry = metadataCache.get(key);
        return cacheEntry.isMissing ? Optional.empty() : Optional.of(new HashMap<>(cacheEntry.value));
    }

    @Override
    public void writeMetadata(String domain, String volumeName, String blobName, Map<String, String> metadata) throws IOException {
        metadataCache.put(
                new MetaKey(domain, volumeName, blobName),
                new CacheEntry<>(new HashMap<>(metadata), true, false));
    }

    @Override
    public void commitMetadata(String domain, String volumeName, String blobName) throws IOException {
        metadataCache.flush(new MetaKey(domain, volumeName, blobName));
    }

    @Override
    public FdsObject readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int maxObjectSize) throws IOException {
        ObjectKey objectKey = new ObjectKey(domain, volumeName, blobName, objectOffset);
        return objectCache.get(objectKey).value;
    }

    @Override
    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, FdsObject fdsObject) throws IOException {
        ObjectKey objectKey = new ObjectKey(domain, volumeName, blobName, objectOffset);
        objectCache.put(objectKey, new CacheEntry<>(fdsObject, true, false));
    }

    @Override
    public void commitObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset) throws IOException {
        ObjectKey objectKey = new ObjectKey(domain, volumeName, blobName, objectOffset);
        objectCache.flush(objectKey);
    }

    @Override
    public Collection<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException {
        Map<MetaKey, BlobMetadata> fromAm = io.scan(domain, volume, blobNamePrefix)
                .stream()
                .collect(Collectors.toMap(bm -> new MetaKey(domain, volume, bm.getBlobName()), bm -> bm));

        Map<MetaKey, BlobMetadata> results = mergeWithCacheEntries(domain, volume, blobNamePrefix, fromAm);
        return results.values();
    }

    private Map<MetaKey, BlobMetadata> mergeWithCacheEntries(String domain, String volume, String blobNamePrefix, Map<MetaKey, BlobMetadata> entries) throws IOException {
        MetaKey prefix = new MetaKey(domain, volume, blobNamePrefix);
        Map<MetaKey, BlobMetadata> results = new HashMap<>(entries);

        SortedMap<MetaKey, CacheEntry<Map<String, String>>> tailmap = metadataCache.tailMap(prefix);
        Iterator<Map.Entry<MetaKey, CacheEntry<Map<String, String>>>> iterator = tailmap.entrySet().iterator();
        while (iterator.hasNext()) {
            Map.Entry<MetaKey, CacheEntry<Map<String, String>>> next = iterator.next();
            if (next != null) {
                MetaKey key = next.getKey();
                CacheEntry<Map<String, String>> cacheEntry = next.getValue();
                if (key.beginsWith(prefix)) {
                    if (cacheEntry.isMissing) {
                        results.remove(key);
                    } else {
                        results.put(key, new BlobMetadata(key.blobName, cacheEntry.value));
                    }
                } else {
                    break;
                }
            }
        }
        return results;
    }

    @Override
    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException {
        MetaKey oldMetaKey = new MetaKey(domain, volumeName, oldName);
        metadataCache.flush(oldMetaKey);
        metadataCache.remove(oldMetaKey);

        ObjectKey objectKey = new ObjectKey(domain, volumeName, oldName);
        SortedMap<ObjectKey, CacheEntry<FdsObject>> tailMap = objectCache.tailMap(objectKey);
        Iterator<ObjectKey> iterator = tailMap.keySet().iterator();

        while (iterator.hasNext()) {
            ObjectKey next = iterator.next();
            if (next.domain.equals(domain) && next.volume.equals(volumeName) && next.blobName.equals(oldName)) {
                objectCache.flush(next);
            }
        }

        io.renameBlob(domain, volumeName, oldName, newName);
    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        MetaKey metaKey = new MetaKey(domain, volume, blobName);
        metadataCache.put(metaKey, new CacheEntry<>(new ConcurrentHashMap<>(), true, true));
        objectCache.dropKeysWithPrefix(new ObjectKey(domain, volume, blobName));
    }

    @Override
    public void commitAll() throws IOException {
        metadataCache.flush();
        objectCache.flush();
    }

    @Override
    public void onVolumeDeletion(String domain, String volumeName) throws IOException {
        LOG.debug("Received volume delete event for [" + volumeName + "], flushing caches");
        metadataCache.dropKeysWithPrefix(new MetaKey(domain, volumeName));
        objectCache.dropKeysWithPrefix(new ObjectKey(domain, volumeName));
    }


    private void doCommitObject(ObjectKey key, CacheEntry<FdsObject> cacheEntry) throws IOException {
        if (cacheEntry.isDirty) {
            io.writeObject(key.domain, key.volume, key.blobName, new ObjectOffset(key.objectOffset), cacheEntry.value);
            cacheEntry.isDirty = false;
        }

    }

    private void doCommitMetadata(MetaKey key, CacheEntry<Map<String, String>> cacheEntry) throws IOException {
        for (IoConsumer<MetaKey> listener : metadataCommitListeners) {
            listener.accept(key);
        }
        if (cacheEntry.isDirty && cacheEntry.isMissing) {
            io.deleteBlob(key.domain, key.volume, key.blobName);
        } else {
            io.writeMetadata(key.domain, key.volume, key.blobName, cacheEntry.value);
            io.commitMetadata(key.domain, key.volume, key.blobName);
        }
        cacheEntry.isDirty = false;
    }
}

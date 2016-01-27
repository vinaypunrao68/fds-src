package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.nfs.deferred.CacheEntry;
import com.formationds.nfs.deferred.EvictingCache;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;


public class DeferredIoOps implements IoOps {
    private static final Logger LOG = Logger.getLogger(DeferredIoOps.class);
    private IoOps io;
    private Counters counters;
    private final EvictingCache<MetaKey, FdsMetadata> metadataCache;
    private final EvictingCache<ObjectKey, FdsObject> objectCache;

    public DeferredIoOps(IoOps io, Counters counters) {
        this.io = io;
        this.counters = counters;
        metadataCache = new EvictingCache<>(
                (key, entry) -> io.writeMetadata(key.domain, key.volume, key.blobName, entry.value, false),
                "Metadata cache",
                100000,
                1,
                TimeUnit.MINUTES);

        objectCache = new EvictingCache<>(
                (key, entry) -> io.writeObject(key.domain, key.volume, key.blobName, new ObjectOffset(key.objectOffset), entry.value, false),
                "Object cache",
                500,
                1,
                TimeUnit.MINUTES);
    }

    public IoOps start() {
        metadataCache.start();
        objectCache.start();
        return this;
    }

    @Override
    public Optional<FdsMetadata> readMetadata(String domain, String volumeName, String blobName) throws IOException {
        MetaKey key = new MetaKey(domain, volumeName, blobName);
        return metadataCache.lock(key, c -> {
            CacheEntry<FdsMetadata> ce = c.get(key);
            if (ce == null) {
                Optional<FdsMetadata> om = io.readMetadata(domain, volumeName, blobName);
                counters.increment(Counters.Key.metadataCacheMiss);
                if (om.isPresent()) {
                    FdsMetadata metadata = om.get();
                    c.put(key, new CacheEntry<>(metadata, false));
                    return Optional.of(metadata);
                } else {
                    return Optional.empty();
                }
            } else {
                counters.increment(Counters.Key.metadataCacheHit);
                return Optional.of(ce.value);
            }
        });
    }

    @Override
    public void writeMetadata(String domain, String volumeName, String blobName, FdsMetadata metadata, boolean deferrable) throws IOException {
        MetaKey key = new MetaKey(domain, volumeName, blobName);
        metadataCache.lock(key, c -> {
            if (deferrable) {
                c.put(key, new CacheEntry<>(metadata, true));
                counters.increment(Counters.Key.deferredMetadataMutation);
            } else {
                io.writeMetadata(domain, volumeName, blobName, metadata, false);
                c.put(key, new CacheEntry<>(metadata, false));
            }
            return null;
        });
    }

    @Override
    public FdsObject readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int maxObjectSize) throws IOException {
        ObjectKey objectKey = new ObjectKey(domain, volumeName, blobName, objectOffset);
        return objectCache.lock(objectKey, c -> {
            CacheEntry<FdsObject> ce = c.get(objectKey);
            if (ce == null) {
                counters.increment(Counters.Key.objectCacheMiss);
                FdsObject o = io.readCompleteObject(domain, volumeName, blobName, objectOffset, maxObjectSize);
                ce = new CacheEntry<>(o, false);
                c.put(objectKey, ce);
            } else {
                counters.increment(Counters.Key.objectCacheHit);
            }

            return ce.value;
        });
    }

    @Override
    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, FdsObject fdsObject, boolean deferrable) throws IOException {
        ObjectKey objectKey = new ObjectKey(domain, volumeName, blobName, objectOffset);
        objectCache.lock(objectKey, c -> {
            counters.increment(Counters.Key.deferredObjectMutation);
            if (deferrable) {
                c.put(objectKey, new CacheEntry<>(fdsObject, true));
            } else {
                io.writeObject(domain, volumeName, blobName, objectOffset, fdsObject, false);
                c.put(objectKey, new CacheEntry<>(fdsObject, false));
            }
            return null;
        });
    }

    @Override
    public Collection<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException {
        Map<MetaKey, BlobMetadata> fromAm = io.scan(domain, volume, blobNamePrefix)
                .stream()
                .collect(Collectors.toMap(bm -> new MetaKey(domain, volume, bm.getBlobName()), bm -> bm));

        MetaKey prefix = new MetaKey(domain, volume, blobNamePrefix);
        Map<MetaKey, BlobMetadata> fromCache = metadataCache.lock(prefix, c -> scanCache(domain, volume, blobNamePrefix));
        fromAm.putAll(fromCache);
        return fromAm.values();
    }

    private Map<MetaKey, BlobMetadata> scanCache(String domain, String volume, String blobNamePrefix) throws IOException {
        MetaKey prefix = new MetaKey(domain, volume, blobNamePrefix);
        Map<MetaKey, BlobMetadata> results = new HashMap<>();

        return metadataCache.lock(prefix, c -> {
            SortedMap<MetaKey, CacheEntry<FdsMetadata>> tailmap = c.tailMap(prefix);
            Iterator<Map.Entry<MetaKey, CacheEntry<FdsMetadata>>> iterator = tailmap.entrySet().iterator();
            while (iterator.hasNext()) {
                Map.Entry<MetaKey, CacheEntry<FdsMetadata>> next = iterator.next();
                if (next != null) {
                    MetaKey key = next.getKey();
                    CacheEntry<FdsMetadata> cacheEntry = next.getValue();
                    if (key.beginsWith(prefix)) {
                        results.put(key, new BlobMetadata(key.blobName, cacheEntry.value));
                    } else {
                        break;
                    }
                }
            }
            return results;
        });
    }

    @Override
    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException {
        MetaKey oldMetaKey = new MetaKey(domain, volumeName, oldName);
        metadataCache.lock(oldMetaKey, mc -> {
                    CacheEntry<FdsMetadata> mce = mc.get(oldMetaKey);
                    if (mce != null && mce.isDirty) {
                        io.writeMetadata(domain, volumeName, oldName, mce.value, false);
                    }
                    mc.remove(oldMetaKey);
                    objectCache.lock(new ObjectKey("", "", "", new ObjectOffset(0)), c -> {
                        HashSet<ObjectKey> keys = new HashSet<>(c.keySet());
                        for (ObjectKey objectKey : keys) {
                            if (objectKey.domain.equals(domain) && objectKey.volume.equals(volumeName) && objectKey.blobName.equals(oldName)) {
                                CacheEntry<FdsObject> oce = c.get(objectKey);
                                if (oce.isDirty) {
                                    io.writeObject(domain, volumeName, oldName, new ObjectOffset(objectKey.objectOffset),
                                            oce.value, false);
                                }
                                c.remove(objectKey);
                            }
                        }
                        return null;
                    });
                    io.renameBlob(domain, volumeName, oldName, newName);
                    return null;
                }
        );
    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        MetaKey metaKey = new MetaKey(domain, volume, blobName);
        metadataCache.lock(metaKey, mc -> {
                    io.deleteBlob(domain, volume, blobName);
                    mc.remove(metaKey);
                    objectCache.lock(new ObjectKey("", "", "", new ObjectOffset(0)), c -> {
                        HashSet<ObjectKey> keys = new HashSet<>(c.keySet());
                        for (ObjectKey key : keys) {
                            if (key.domain.equals(domain) && key.volume.equals(volume) && key.blobName.equals(blobName)) {
                                c.remove(key);
                            }
                        }
                        return null;
                    });
                    return null;
                }
        );
    }


    @Override
    public void flush() throws IOException {
        metadataCache.flush();
        objectCache.flush();
    }

    @Override
    public void onVolumeDeletion(String domain, String volumeName) throws IOException {
        LOG.debug("Received volume delete event for [" + volumeName + "], flushing caches");
        metadataCache.flush(new MetaKey(domain, volumeName, ""));
        objectCache.flush(new ObjectKey(domain, volumeName, "", new ObjectOffset(0)));
    }

}

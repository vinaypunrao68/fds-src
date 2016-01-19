package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.nfs.deferred.CacheEntry;
import com.formationds.nfs.deferred.EvictingCache;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;


public class DeferredIoOps implements IoOps {
    private static final Logger LOG = Logger.getLogger(DeferredIoOps.class);
    private IoOps io;
    private Counters counters;
    private final EvictingCache<MetaKey, Map<String, String>> metadataCache;
    private final EvictingCache<ObjectKey, ByteBuffer> objectCache;

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
                (key, entry) -> io.writeObject(key.domain, key.volume, key.blobName, new ObjectOffset(key.objectOffset), entry.value, entry.value.remaining(), false),
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
    public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException {
        MetaKey key = new MetaKey(domain, volumeName, blobName);
        return metadataCache.lock(key, c -> {
            CacheEntry<Map<String, String>> ce = c.get(key);
            if (ce == null) {
                Optional<Map<String, String>> om = io.readMetadata(domain, volumeName, blobName);
                if (om.isPresent()) {
                    c.put(key, new CacheEntry<>(new HashMap<>(om.get()), false));
                }
                counters.increment(Counters.Key.metadataCacheMiss);
                return om;
            } else {
                counters.increment(Counters.Key.metadataCacheHit);
                return Optional.of(ce.value);
            }
        });
    }

    @Override
    public void writeMetadata(String domain, String volumeName, String blobName, Map<String, String> metadata, boolean deferrable) throws IOException {
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
    public ByteBuffer readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int objectSize) throws IOException {
        ObjectKey objectKey = new ObjectKey(domain, volumeName, blobName, objectOffset);
        return objectCache.lock(objectKey, c -> {
            CacheEntry<ByteBuffer> ce = c.get(objectKey);
            if (ce == null) {
                counters.increment(Counters.Key.objectCacheMiss);
                ByteBuffer buf = io.readCompleteObject(domain, volumeName, blobName, objectOffset, objectSize);
                ce = new CacheEntry<>(buf, false);
                c.put(objectKey, ce);
            } else {
                counters.increment(Counters.Key.objectCacheHit);
            }

            return ce.value;
        });
    }

    @Override
    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, ByteBuffer byteBuffer, int objectSize, boolean deferrable) throws IOException {
        ObjectKey objectKey = new ObjectKey(domain, volumeName, blobName, objectOffset);
        objectCache.lock(objectKey, c -> {
            counters.increment(Counters.Key.deferredObjectMutation);
            if (deferrable) {
                c.put(objectKey, new CacheEntry<>(byteBuffer, true));
            } else {
                io.writeObject(domain, volumeName, blobName, objectOffset, byteBuffer.slice().duplicate(), objectSize, false);
                c.put(objectKey, new CacheEntry<>(byteBuffer.slice(), false));
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
            SortedMap<MetaKey, CacheEntry<Map<String, String>>> tailmap = c.tailMap(prefix);
            Iterator<MetaKey> iterator = tailmap.keySet().iterator();
            while (iterator.hasNext()) {
                MetaKey next = iterator.next();
                if (next.beginsWith(prefix)) {
                    results.put(next, new BlobMetadata(next.blobName, c.get(next).value));
                } else {
                    break;
                }
            }
            return results;
        });
    }

    @Override
    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException {
        MetaKey oldMetaKey = new MetaKey(domain, volumeName, oldName);
        metadataCache.lock(oldMetaKey, mc -> {
                    CacheEntry<Map<String, String>> mce = mc.get(oldMetaKey);
                    if (mce != null && mce.isDirty) {
                        io.writeMetadata(domain, volumeName, oldName, mce.value, false);
                    }
                    mc.remove(oldMetaKey);
                    objectCache.lock(new ObjectKey("", "", "", new ObjectOffset(0)), c -> {
                        HashSet<ObjectKey> keys = new HashSet<>(c.keySet());
                        for (ObjectKey objectKey : keys) {
                            if (objectKey.domain.equals(domain) && objectKey.volume.equals(volumeName) && objectKey.blobName.equals(oldName)) {
                                CacheEntry<ByteBuffer> oce = c.get(objectKey);
                                if (oce.isDirty) {
                                    io.writeObject(domain, volumeName, oldName, new ObjectOffset(objectKey.objectOffset),
                                            oce.value, oce.value.remaining(), false);
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
                    io.deleteBlob(domain, volume, blobName);
                    return null;
                }
        );
    }


    @Override
    public void flush() throws IOException {
        metadataCache.flush();
        objectCache.flush();
    }
}

package com.formationds.nfs;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

public class Counters {
    private ConcurrentHashMap<Key, AtomicLong> map;

    public Counters() {
        map = new ConcurrentHashMap<>();
    }

    public static enum Key {
        inodeCreate,
        inodeAccess,
        lookupFile,
        link,
        listDirectory,
        mkdir,
        move,
        read,
        bytesRead,
        readLink,
        remove,
        symlink,
        write,
        bytesWritten,
        getAttr,
        setAttr,
        metadataCacheMiss,
        metadataCacheHit,
        objectCacheMiss,
        objectCacheHit,
        AM_statBlob,
        deferredMetadataMutation,
        AM_updateMetadataTx,
        AM_metadata_flush,
        AM_updateBlobOnce_metadataOnly,
        AM_getBlobWithMeta,
        AM_updateBlobOnce_objectAndMetadata,
        AM_updateBlob;
    }

    public long reset(Key key) {
        long[] oldValue = new long[1];
        map.compute(key, (k, v) -> {
            if (v == null) {
                v = new AtomicLong(0);
            }

            oldValue[0] = v.get();
            v.set(0);
            return v;
        });
        return oldValue[0];
    }


    public Map<Key, Long> harvest() {
        Map<Key, Long> result = new HashMap<>();
        for (Key key : Key.values()) {
            map.compute(key, (k, v) -> {
                if (v == null) {
                    v = new AtomicLong();
                }

                result.put(key, v.getAndSet(0));
                return v;
            });
        }
        return result;
    }

    public long get(Key key) {
        return map.getOrDefault(key, new AtomicLong(0)).get();
    }

    public void increment(Key key) {
        map.compute(key, (k, v) -> {
            if (v == null) {
                v = new AtomicLong(0);
            }

            v.incrementAndGet();
            return v;
        });
    }

    public void increment(Key key, long howMany) {
        map.compute(key, (k, v) -> {
            if (v == null) {
                v = new AtomicLong(0);
            }

            v.addAndGet(howMany);
            return v;
        });
    }

    public void decrement(Key key) {
        map.compute(key, (k, v) -> {
            if (v == null) {
                v = new AtomicLong(0);
            }

            v.decrementAndGet();
            return v;
        });
    }
}

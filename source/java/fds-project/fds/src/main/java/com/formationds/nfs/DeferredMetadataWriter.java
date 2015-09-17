package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.RemovalCause;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.*;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class DeferredMetadataWriter implements Io {
    private static final Logger LOG = Logger.getLogger(DeferredMetadataWriter.class);
    private DirectAmIo io;
    private AsyncAm asyncAm;
    private Counters counters;
    private BlockingDeque<IOException> exceptions;
    private Executor executor;
    private Cache<Key, Value> cache;
    private Cache<Key, Object> locks;

    public DeferredMetadataWriter(DirectAmIo io, AsyncAm asyncAm, Counters counters) {
        this.io = io;
        this.asyncAm = asyncAm;
        this.counters = counters;
        exceptions = new LinkedBlockingDeque<>();
        executor = Executors.newCachedThreadPool();
        cache = CacheBuilder.newBuilder()
                .maximumSize(100000)
                .expireAfterAccess(1, TimeUnit.MINUTES)
                .removalListener(notification -> {
                    RemovalCause cause = notification.getCause();
                    if (!(cause.equals(RemovalCause.EXPLICIT) || cause.equals(RemovalCause.REPLACED))) {
                        executor.execute(() -> flush((Key) notification.getKey(), (Value) notification.getValue()));
                    }
                }).build();

        locks = CacheBuilder.newBuilder()
                .maximumSize(100000)
                .expireAfterAccess(1, TimeUnit.MINUTES)
                .build();
    }

    public void start() {
        Thread thread = new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    break;
                }
                cache.cleanUp();
                locks.cleanUp();
            }
        });
        thread.setName("NFS metadata updates writer");
        thread.start();
    }

    @Override
    public <T> T mapMetadata(String domain, String volumeName, String blobName, MetadataMapper<T> mapper) throws IOException {
        bubbleExceptions();
        Key key = new Key(domain, volumeName, blobName);
        synchronized (lock(key)) {
            Value value = cache.getIfPresent(key);
            if (value == null) {
                Optional<Map<String, String>> fromAm = io.mapMetadata(domain, volumeName, blobName, om -> om);
                if (!fromAm.isPresent()) {
                    return mapper.map(fromAm);
                } else {
                    value = new Value(fromAm.get(), false);
                    cache.put(key, value);
                }
            }
            return mapper.map(Optional.of(value.map));
        }
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, MetadataMutator mutator) throws IOException {
        Key key = new Key(domain, volume, blobName);
        mapMetadata(domain, volume, blobName, metadata -> {
            Map<String, String> m = new HashMap<>(metadata.orElse(new HashMap<>()));
            mutator.mutate(m);

            // If the object is not present in the catalog, create it, otherwise we can't list directory contents
            if (!metadata.isPresent()) {
                io.mutateMetadata(domain, volume, blobName, m, false);
                cache.put(key, new Value(m, false));
            } else {
                cache.put(key, new Value(m, true));
                counters.increment(Counters.Key.deferredMetadataMutation);
            }

            return null;
        });
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, Map<String, String> map, boolean deferrable) throws IOException {
        bubbleExceptions();
        Key key = new Key(domain, volume, blobName);
        synchronized (lock(key)) {
            if (deferrable) {
                cache.put(key, new Value(map, true));
                counters.increment(Counters.Key.deferredMetadataMutation);
            } else {
                io.mutateMetadata(domain, volume, blobName, map, false);
                cache.put(key, new Value(map, false));
            }
        }
    }

    @Override
    public <T> T mapObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMapper<T> objectMapper) throws IOException {
        bubbleExceptions();
        Key key = new Key(domain, volume, blobName);

        // Read object and metadata from AM, replace metadata with the cached one
        return io.mapObjectAndMetadata(domain, volume, blobName, objectSize, objectOffset, new ObjectMapper<T>() {
            @Override
            public T map(Optional<ObjectView> oov) throws IOException {
                Map<String, String> metadata = null;

                synchronized (lock(key)) {
                    if (!oov.isPresent()) {
                        cache.asMap().remove(key);
                        return objectMapper.map(oov);
                    }
                    metadata = oov.get().getMetadata();
                    Value value = cache.asMap().putIfAbsent(key, new Value(metadata, true));
                    return objectMapper.map(Optional.of(new ObjectView(value.map, oov.get().getBuf())));
                }
            }
        });
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMutator mutator) throws IOException {
        throw new UnsupportedOperationException();
    }

    /**
     * Bypass AM by writing this metadata update in our cache, pass the object mutation along to AM
     */
    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, Map<String, String> metadata) throws IOException {
        bubbleExceptions();
        Key key = new Key(domain, volume, blobName);

        try {
            unwindExceptions(() -> {
                TxDescriptor tx = asyncAm.startBlobTx(domain, volume, blobName, 0).get();
                asyncAm.updateBlob(domain, volume, blobName, tx, byteBuffer, objectSize, objectOffset, false).get();
                asyncAm.commitBlobTx(domain, volume, blobName, tx).get();
                counters.increment(Counters.Key.AM_updateBlob);
                return null;
            });

            synchronized (lock(key)) {
                cache.put(key, new Value(metadata, true));
                counters.increment(Counters.Key.deferredMetadataMutation);
            }

        } catch (ApiException e) {
            LOG.error("AM.updateBlob() TX returned error code " + e.getErrorCode() + ", volume=" + key.volume + ", blobName=" + key.blobName, e);
            throw new IOException(e);
        } catch (IOException e) {
            LOG.error("AM.updateBlob() TX got an IOException", e);
            throw e;
        } catch (Exception e) {
            LOG.error("AM.updateMetadata() TX got an Exception", e);
            throw new IOException(e);
        }
    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        bubbleExceptions();
        Key key = new Key(domain, volume, blobName);
        synchronized (lock(key)) {
            cache.asMap().remove(new Key(domain, volume, blobName));
        }
        io.deleteBlob(domain, volume, blobName);
    }

    @Override
    public <T> List<T> scan(String domain, String volume, String blobNamePrefix, MetadataMapper<T> mapper) throws IOException {
        bubbleExceptions();
        return io.scan(domain, volume, blobNamePrefix, mapper);
    }

    private Object lock(Key key) {
        try {
            return locks.get(key, () -> new Object());
        } catch (ExecutionException e) {
            LOG.error("Error acquiring lock", e);
            throw new RuntimeException(e);
        }
    }

    private void bubbleExceptions() throws IOException {
        List<IOException> exs = new ArrayList<>();
        exceptions.drainTo(exs);
        if (exs.size() != 0) {
            throw exs.get(0);
        }
    }

    private void flush(Key key, Value value) {
        if (value.isDirty) {
            try {
                unwindExceptions(() -> {
                    TxDescriptor tx = asyncAm.startBlobTx(key.domain, key.volume, key.blobName, 0).get();
                    asyncAm.updateMetadata(key.domain, key.volume, key.blobName, tx, value.map).get();
                    asyncAm.commitBlobTx(key.domain, key.volume, key.blobName, tx);
                    counters.increment(Counters.Key.AM_metadata_flush);
                    return null;
                });
                value.isDirty = false;
            } catch (ApiException e) {
                if (!e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                    LOG.error("AM.updateMetadata() TX returned error code " + e.getErrorCode() + ", volume=" + key.volume + ", blobName=" + key.blobName, e);
                    exceptions.add(new IOException(e));
                }
            } catch (IOException e) {
                LOG.error("AM.updateMetadata() TX got an IOException", e);
                exceptions.add(e);
            } catch (Exception e) {
                LOG.error("AM.updateMetadata() TX got an Exception", e);
                exceptions.add(new IOException(e));
            }
        }
    }

    private static class Value {
        Map<String, String> map;
        boolean isDirty;

        Value(Map<String, String> map, boolean isDirty) {
            this.map = map;
            this.isDirty = isDirty;
        }
    }

    private static class Key {
        String domain;
        String volume;
        String blobName;

        Key(String domain, String volume, String blobName) {
            this.domain = domain;
            this.volume = volume;
            this.blobName = blobName;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Key key = (Key) o;

            if (!blobName.equals(key.blobName)) return false;
            if (!domain.equals(key.domain)) return false;
            if (!volume.equals(key.volume)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = domain.hashCode();
            result = 31 * result + volume.hashCode();
            result = 31 * result + blobName.hashCode();
            return result;
        }
    }
}

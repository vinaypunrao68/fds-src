package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.util.blob.Mode;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Collectors;

public class MemoryAmService implements AmService.Iface {
    class Tx {
        private final Map<Long, byte[]> data;
        private long id;
        private int mode;
        private Map<String, String> metadata;
        private int objectSize;
        private String volume;
        private String blob;

        private Tx(String volume, String blob, long id, int objectSize, int mode) {
            this.volume = volume;
            this.blob = blob;
            this.id = id;
            this.objectSize = objectSize;
            this.mode = mode;
            data = new HashMap<>();
            metadata = null;
        }

        private void updateMetdata(Map<String, String> metadata) {
            this.metadata = metadata;
        }

        private void putObject(long offset, byte[] bytes) throws ApiException {
            synchronized (data) {
                if(bytes.length > objectSize)
                    throw new ApiException("request data exceeds object size", ErrorCode.BAD_REQUEST);
                data.put(offset, bytes);
            }
        }
    }

    class Blob {
        private final Map<Long, byte[]> data;
        private String name;
        private Volume volume;
        private Map<String, String> metadata;

        private BlobDescriptor getDescriptor() {
            return new BlobDescriptor(name, data.values().stream().collect(Collectors.summingLong(v -> v.length)), metadata);
        }

        private void apply(Tx tx) throws ApiException {
            synchronized (data) {
                data.putAll(tx.data);
                if(tx.metadata != null)
                    metadata = tx.metadata;
                if(tx.mode == Mode.TRUNCATE.getValue()) {
                    long max = tx.data.keySet().stream().max((i, j) -> Long.compare(i, j)).get();
                    data.entrySet().removeIf(d -> d.getKey() > max);
                }
            }
        }

        private byte[] getObject(long offset) throws ApiException {
            synchronized (data) {
                if(!data.containsKey(offset))
                    throw new ApiException("no object found at offset " + offset, ErrorCode.MISSING_RESOURCE);
                return data.get(offset);
            }
        }

        private Blob(String name) {
            this.name = name;
            this.data = new HashMap<>();
        }
    }

    class Volume {
        private VolumeSettings settings;
        private String name;

        private final Map<String, Blob> blobMap;

        private Volume(String name, VolumeSettings settings) {
            this.name = name;
            this.settings = settings;
            blobMap = new HashMap<>();
        }

        private Blob getBlob(String name) throws ApiException {
            synchronized (blobMap) {
                if(!blobMap.containsKey(name))
                    throw new ApiException("Blob does not exist", ErrorCode.MISSING_RESOURCE);
                return blobMap.get(name);
            }
        }

        private void applyTx(String blobName, Tx tx) throws ApiException {
            synchronized (blobMap) {
                Blob blob = blobMap.computeIfAbsent(blobName, n -> new Blob(n));
                blob.apply(tx);
            }
        }

        public void deleteBlob(String blob) {
            synchronized (blobMap) {
                blobMap.remove(blob);
            }
        }

        public VolumeStatus toVolumeStatus() {
            return new VolumeStatus(blobMap.size(), blobMap.values().stream().collect(Collectors.summingLong(b -> b.getDescriptor().byteCount)));
        }
    }

    private AtomicLong txId;
    private final HashMap<String, Volume> volumesMap;
    private final HashMap<Long, Tx> txMap;

    private long getNextTxId() { return txId.incrementAndGet(); }

    public MemoryAmService() {
        volumesMap = new HashMap<>();
        txMap = new HashMap<>();
        txId = new AtomicLong(0);
    }

    public void createVolume(String volumeName, VolumeSettings settings) throws ApiException {
        synchronized (volumesMap) {
            if(volumesMap.containsKey(volumeName))
                throw new ApiException("volume already exists", ErrorCode.RESOURCE_ALREADY_EXISTS);
            volumesMap.put(volumeName, new Volume(volumeName, settings));
        }
    }

    private Volume getVolume(String volumeName) throws ApiException {
        synchronized (volumesMap) {
            if(!volumesMap.containsKey(volumeName))
                throw new ApiException("volume does not exist", ErrorCode.MISSING_RESOURCE);
            return  volumesMap.get(volumeName);
        }
    }

    private Tx getTx(String volume, String blob, long id) throws ApiException {
        synchronized (txMap) {
            if(!txMap.containsKey(id)) {
                throw new ApiException("transaction id does not exist", ErrorCode.MISSING_RESOURCE);
            }
            Tx tx = txMap.get(id);
            // TODO: test this in the real platform
            if(!tx.blob.equals(blob) || !tx.volume.equals(volume))
                throw new ApiException("transaction id does not correspond to an operation on this blob/volume", ErrorCode.BAD_REQUEST);
            return tx;
        }
    }

    @Override
    public void attachVolume(String s, String s2) throws ApiException, TException {
        // no-op
    }

    @Override
    public List<BlobDescriptor> volumeContents(String domain, String volume, int count, long offset) throws ApiException, TException {
        return getVolume(volume).blobMap.entrySet().stream()
                .sorted((i, j) -> i.getKey().compareTo(j.getKey()))
                .skip(offset)
                .limit(count)
                .map(b -> b.getValue().getDescriptor())
                .collect(Collectors.toList());
    }

    @Override
    public BlobDescriptor statBlob(String domain, String volume, String blob) throws ApiException, TException {
        return getVolume(volume).getBlob(blob).getDescriptor();
    }

    @Override
    public TxDescriptor startBlobTx(String domain, String volume, String blob, int mode) throws ApiException, TException {
        synchronized (txMap) {
            Tx tx = new Tx(volume, blob, getNextTxId(), getVolume(volume).settings.getMaxObjectSizeInBytes(), mode);
            txMap.put(tx.id, tx);
            return new TxDescriptor(tx.id);
        }
    }

    @Override
    public void commitBlobTx(String domain, String volume, String blob, TxDescriptor txDescriptor) throws ApiException, TException {
        synchronized (txMap) {
            Tx tx = getTx(volume, blob, txDescriptor.getTxId());
            getVolume(volume).applyTx(blob, tx);
            txMap.remove(tx.id);
        }
    }

    @Override
    public void abortBlobTx(String domain, String volume, String blob, TxDescriptor txDescriptor) throws ApiException, TException {
        synchronized (txMap) {
            // TODO: is this really idempotent in the platform
            txMap.remove(txDescriptor.getTxId());
        }
    }

    @Override
    public ByteBuffer getBlob(String domain, String volume, String blob, int length, ObjectOffset objectOffset) throws ApiException, TException {
        byte[] object = getVolume(volume).getBlob(blob).getObject(objectOffset.getValue());
        return ByteBuffer.wrap(object, 0, Math.min(length, object.length));
    }

    @Override
    public void updateMetadata(String domain, String volume, String blob, TxDescriptor txDescriptor, Map<String, String> stringStringMap) throws ApiException, TException {
        getTx(volume, blob, txDescriptor.getTxId()).updateMetdata(stringStringMap);
    }

    @Override
    public void updateBlob(String domain, String volume, String blob, TxDescriptor txDescriptor, ByteBuffer buffer, int length, ObjectOffset objectOffset, boolean ignored) throws ApiException, TException {
        byte[] object = new byte[length];
        buffer.get(object);
        getTx(volume, blob, txDescriptor.getTxId()).putObject(objectOffset.getValue(), object);
    }

    @Override
    public void updateBlobOnce(String domain, String volume, String blob, int mode, ByteBuffer buffer, int length, ObjectOffset objectOffset, Map<String, String> stringStringMap) throws ApiException, TException {
        TxDescriptor descriptor = startBlobTx(domain, volume, blob, mode);
        updateBlob(domain, volume, blob, descriptor, buffer, length, objectOffset, false);
        updateMetadata(domain, volume, blob, descriptor, stringStringMap);
        commitBlobTx(domain, volume, blob, descriptor);
    }

    @Override
    public void deleteBlob(String domain, String volume, String blob) throws ApiException, TException {
        getVolume(volume).deleteBlob(blob);
    }

    @Override
    public VolumeStatus volumeStatus(String domain, String volume) throws ApiException, TException {
        return getVolume(volume).toVolumeStatus();
    }
}

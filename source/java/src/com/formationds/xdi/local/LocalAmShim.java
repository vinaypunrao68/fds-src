package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.*;
import org.apache.thrift.TException;
import org.hibernate.criterion.Projections;
import org.hibernate.criterion.Restrictions;
import org.json.JSONObject;

import java.io.File;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

public class LocalAmShim implements AmShim.Iface {

    private Persister persister;
    private final Map<UUID, TxState> blobTxs;

    private class TxState {
        long blobId;
        long currentByteCount;
        int blockSize;

        private TxState(long blobId, long currentByteCount, int blockSize) {
            this.blobId = blobId;
            this.currentByteCount = currentByteCount;
            this.blockSize = blockSize;
        }
    }

    public LocalAmShim(String memoryDbName) {
        persister = new Persister(memoryDbName);
        blobTxs = new ConcurrentHashMap<>();
    }

    public LocalAmShim(File dbPath) {
        persister = new Persister(dbPath);
        blobTxs = new ConcurrentHashMap<>();
    }

    public void createDomain(String domainName) {
        persister.create(new Domain(domainName));
    }

    @Override
    public void createVolume(String domainName, String volumeName, VolumePolicy volumePolicy) throws XdiException, TException {
        Domain domain = (Domain) persister.execute(session -> session.createCriteria(Domain.class)
                .add(Restrictions.eq("name", domainName))
                .uniqueResult());
        persister.create(new Volume(domain, volumeName, volumePolicy.getObjectSizeInBytes()));
    }

    @Override
    public void deleteVolume(String domainName, String volumeName) throws XdiException, TException {
        Volume volume = getVolume(domainName, volumeName);
        List<Blob> blobs = getAllVolumeBlobs(domainName, volumeName);
        for (Blob blob : blobs) {
            List<Block> blocks = persister.execute(session ->
                    session.createCriteria(Block.class)
                            .add(Restrictions.eq("blobId", blob.getId())))
                            .list();
            blocks.forEach(b -> persister.delete(b));
        }

        persister.delete(volume);
    }

    private Volume getVolume(String domainName, String volumeName) {
        return (Volume) persister.execute(session ->
                session.createCriteria(Volume.class)
                        .add(Restrictions.eq("name", volumeName))
                        .createCriteria("domain")
                        .add(Restrictions.eq("name", domainName))
                        .uniqueResult());
    }

    @Override
    public VolumeDescriptor statVolume(String domainName, String volumeName) throws XdiException, TException {
        Volume volume = getVolume(domainName, volumeName);
        Uuid uuid = new Uuid(volume.getUuidLow(), volume.getUuidHigh());
        return new VolumeDescriptor(volumeName, uuid, volume.getTimestamp(), new VolumePolicy(volume.getObjectSize()));
    }

    @Override
    public List<VolumeDescriptor> listVolumes(String domainName) throws XdiException, TException {
        List<Volume> volumes = persister.execute(session ->
                session.createCriteria(Volume.class)
                        .createCriteria("domain")
                        .add(Restrictions.eq("name", domainName))
                        .list());

        return volumes.stream()
                .map(v -> new VolumeDescriptor(v.getName(), new Uuid(v.getUuidLow(), v.getUuidHigh()), v.getTimestamp(), new VolumePolicy(v.getObjectSize())))
                .collect(Collectors.toList());
    }


    @Override
    public List<BlobDescriptor> volumeContents(String domainName, String volumeName, int count, long offset) throws XdiException, TException {
        List<Blob> blobs = getAllVolumeBlobs(domainName, volumeName);

        return blobs.stream()
                .skip(offset)
                .limit(count)
                .map(b -> new BlobDescriptor(b.getName(), b.getByteCount(), b.getMetadata()))
                .collect(Collectors.toList());
    }

    private List<Blob> getAllVolumeBlobs(String domainName, String volumeName) {
        return persister.execute(session ->
                    session.createCriteria(Blob.class)
                            .createCriteria("volume")
                            .add(Restrictions.eq("name", volumeName))
                            .createCriteria("domain")
                            .add(Restrictions.eq("name", domainName))
                            .list());
    }

    @Override
    public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws XdiException, TException {
        Blob blob = getBlob(domainName, volumeName, blobName);
        return new BlobDescriptor(blobName, blob.getByteCount(), blob.getMetadata());
    }

    @Override
    public Uuid startBlobTx(String domainName, String volumeName, String blobName) throws XdiException, TException {
        UUID uuid = UUID.randomUUID();
        Blob blob = getOrCreate(domainName, volumeName, blobName);
        long blobId = blob.getId();
        long byteCount = blob.getByteCount();
        blobTxs.put(uuid, new TxState(blobId, byteCount, blob.getVolume().getObjectSize()));
        return new Uuid(uuid.getLeastSignificantBits(), uuid.getMostSignificantBits());
    }

    @Override
    public void commit(Uuid txId) throws XdiException, TException {
        UUID uuid = new UUID(txId.getHigh(), txId.getLow());
        blobTxs.computeIfPresent(uuid, (k, v) -> {
            Blob blob = persister.load(Blob.class, v.blobId);
            blob.setByteCount(v.currentByteCount);
            persister.update(blob);
            return v;
        });
        blobTxs.remove(uuid);
    }

    @Override
    public VolumeStatus volumeStatus(String domainName, String volumeName) throws XdiException, TException {
        long count = (int) persister.execute(session ->
                session.createCriteria(Blob.class)
                        .createCriteria("volume")
                        .add(Restrictions.eq("name", volumeName))
                        .createCriteria("domain")
                        .add(Restrictions.eq("name", domainName))
                        .setProjection(Projections.rowCount()))
                .uniqueResult();
        return new VolumeStatus(count);
    }

    @Override
    public ByteBuffer getBlob(String domainName, String volumeName, String blobName, int length, long offset) throws XdiException, TException {
        Blob blob = getBlob(domainName, volumeName, blobName);
        int objectSize = blob.getVolume().getObjectSize();

        byte[] read = new BlockReader().read(i -> {
            return getOrMakeBlock(i, blob.getId(), objectSize).getBytes();
        }, objectSize, offset, length);
        return ByteBuffer.wrap(read);
    }

    @Override
    public void updateMetadata(String domainName, String volumeName, String blobName, Uuid txUuid, Map<String, String> metadata) throws XdiException, TException {
        Blob blob = getOrCreate(domainName, volumeName, blobName);
        blob.setMetadataJson(new JSONObject(metadata).toString(2));
        persister.update(blob);
    }


    @Override
    public void updateBlob(String domainName, String volumeName, String blobName, Uuid txUuid, ByteBuffer bytes, int length, long offset) throws XdiException, TException {
        UUID uuid = new UUID(txUuid.getHigh(), txUuid.getLow());
        blobTxs.computeIfPresent(uuid, (k, state) -> {
            state.currentByteCount = Math.max(state.currentByteCount, offset + length);
            BlockWriter writer = new BlockWriter(i -> getOrMakeBlock(i, state.blobId, state.blockSize), state.blockSize);
            Iterator<Block> updated = writer.update(bytes.array(), length, offset);
            while (updated.hasNext()) {
                Block next = updated.next();
                if (next.getId() == -1) {
                    persister.create(next);
                } else {
                    persister.update(next);
                }
            }
            return state;
        });
    }

    @Override
    public void deleteBlob(String domainName, String volumeName, String blobName) throws XdiException, TException {
        Blob blob = getBlob(domainName, volumeName, blobName);
        List<Block> blocks = persister.execute(session -> {
            return session.createCriteria(Block.class)
                    .add(Restrictions.eq("blobId", blob.getId()))
                    .list();
        });
        for (Block block : blocks) {
            persister.delete(block);
        }
        persister.delete(blob);
    }

    private Block getOrMakeBlock(long position, long blobId, int blockSize) {
        Block block = (Block) persister.execute(session ->
                session.createCriteria(Block.class)
                        .add(Restrictions.eq("blobId", blobId))
                        .add(Restrictions.eq("position", position))
                        .uniqueResult());

        if (block == null) {
            return new Block(blobId, position, new byte[blockSize]);
        } else {
            return block;
        }
    }


    private Blob getOrCreate(String domainName, String volumeName, String blobName) {
        Blob blob = getBlob(domainName, volumeName, blobName);
        if (blob == null) {
            blob = persister.create(new Blob(getVolume(domainName, volumeName), blobName));
        }
        return blob;
    }

    public Blob getBlob(String domainName, String volumeName, String blobName) {
        return persister.execute(session -> {
            Blob blob = (Blob) session
                    .createCriteria(Blob.class)
                    .add(Restrictions.eq("name", blobName))
                    .createCriteria("volume")
                    .add(Restrictions.eq("name", volumeName))
                    .createCriteria("domain")
                    .add(Restrictions.eq("name", domainName))
                    .uniqueResult();

            if (blob != null) {
                blob.initialize();
            }
            return blob;
        });
    }
}

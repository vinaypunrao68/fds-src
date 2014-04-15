package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.*;
import org.apache.thrift.TException;
import org.hibernate.criterion.Projections;
import org.hibernate.criterion.Restrictions;
import org.json.JSONObject;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class LocalAmShim implements AmShim.Iface {

    private Persister persister;

    public LocalAmShim() {
        persister = new Persister("local");
    }

    public void createDomain(String domainName) {
        persister.create(new Domain(domainName));
    }

    @Override
    public void createVolume(String domainName, String volumeName, VolumePolicy volumePolicy) throws FdsException, TException {
        Domain domain = (Domain) persister.execute(session -> session.createCriteria(Domain.class)
                .add(Restrictions.eq("name", domainName))
                .uniqueResult());
        persister.create(new Volume(domain, volumeName, volumePolicy.getObjectSizeInBytes()));
    }

    @Override
    public void deleteVolume(String domainName, String volumeName) throws FdsException, TException {
        Volume volume = getVolume(domainName, volumeName);

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
    public VolumeDescriptor statVolume(String domainName, String volumeName) throws FdsException, TException {
        Volume volume = getVolume(domainName, volumeName);
        Uuid uuid = new Uuid(volume.getUuidLow(), volume.getUuidHigh());
        return new VolumeDescriptor(volumeName, uuid, volume.getTimestamp(), new VolumePolicy(volume.getObjectSize()));
    }

    @Override
    public List<VolumeDescriptor> listVolumes(String domainName) throws FdsException, TException {
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
    public List<String> volumeContents(String domainName, String volumeName, int count, long offset) throws FdsException, TException {
        List<Blob> blobs = persister.execute(session ->
                session.createCriteria(Blob.class)
                        .createCriteria("volume")
                        .add(Restrictions.eq("name", volumeName))
                        .createCriteria("domain")
                        .add(Restrictions.eq("name", domainName))
                        .list());

        return blobs.stream()
                .skip(offset)
                .limit(count)
                .map(b -> b.getName())
                .collect(Collectors.toList());
    }

    @Override
    public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws FdsException, TException {
        Blob blob = getBlob(domainName, volumeName, blobName);
        Map<String, String> metadata = new HashMap<>();
        JSONObject o = new JSONObject(blob.getMetadataJson());
        Iterator<String> keys = o.keys();
        while (keys.hasNext()) {
            String k = keys.next();
            metadata.put(k, o.getString(k));
        }
        return new BlobDescriptor(blob.getByteCount(), metadata);
    }

    @Override
    public Uuid startBlobTx(String domainName, String volumeName, String blobName) throws FdsException, TException {
        return new Uuid(0, 0);
    }

    @Override
    public void commit(Uuid txId) throws FdsException, TException {

    }

    @Override
    public VolumeStatus volumeStatus(String domainName, String volumeName) throws FdsException, TException {
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
    public ByteBuffer getBlob(String domainName, String volumeName, String blobName, int length, long offset) throws FdsException, TException {
        Blob blob = getBlob(domainName, volumeName, blobName);
        int objectSize = blob.getVolume().getObjectSize();
        List<Block> blocks = blob.getBlocks();

        byte[] read = new BlockReader().read(i -> {
            return i >= blocks.size()  ? new byte[objectSize] : blob.getBlocks().get(i).getBytes();
        }, objectSize, offset, length);
        return ByteBuffer.wrap(read);
    }

    @Override
    public void updateMetadata(String domainName, String volumeName, String blobName, Uuid txUuid, Map<String, String> metadata) throws FdsException, TException {
        Blob blob = getOrCreate(domainName, volumeName, blobName);
        blob.setMetadataJson(new JSONObject(metadata).toString(2));
        persister.update(blob);
    }


    @Override
    public void updateBlob(String domainName, String volumeName, String blobName, Uuid txUuid, ByteBuffer bytes, int length, long offset) throws FdsException, TException {
        Blob blob = getOrCreate(domainName, volumeName, blobName);
        long byteCount = Math.max(blob.getByteCount(), offset + length);
        List<Block> blocks = blob.getBlocks();
        BlockWriter writer = new BlockWriter(i -> getOrMakeBlock(blob, blocks, i), blob.getVolume().getObjectSize());
        Iterator<Block> updated = writer.update(bytes.array(), length, offset);
        while (updated.hasNext()) {
            Block next = updated.next();
            if (next.getId() == -1) {
                persister.create(next);
            } else {
                persister.update(next);
            }
        }

        if (byteCount > blob.getByteCount()) {
            blob.setByteCount(byteCount);
            persister.update(blob);
        }
    }

    @Override
    public void deleteBlob(String domainName, String volumeName, String blobName) throws FdsException, TException {
        Blob blob = getBlob(domainName, volumeName, blobName);
        for (Block block : blob.getBlocks()) {
            persister.delete(block);
        }
        persister.delete(blob);
    }

    private Block getOrMakeBlock(Blob blob, List<Block> blocks, Integer i) {
        return i >= blocks.size() ? new Block(blob, new byte[blob.getVolume().getObjectSize()]) : blocks.get(i);
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

package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import org.apache.thrift.TException;
import org.hibernate.criterion.Projections;
import org.hibernate.criterion.Restrictions;
import org.json.JSONObject;

import java.io.File;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.stream.Collectors;

public class ToyServices implements AmService.Iface, ConfigurationService.Iface {

    private Persister persister;

    public ToyServices(String memoryDbName) {
        persister = new Persister(memoryDbName);
    }

    public ToyServices(File dbPath) {
        persister = new Persister(dbPath);
    }

    public void createDomain(String domainName) {
        persister.create(new Domain(domainName));
    }

    @Override
    public void createVolume(String domainName, String volumeName, VolumePolicy volumePolicy) throws ApiException, TException {
        Domain domain = (Domain) persister.execute(session -> session.createCriteria(Domain.class)
                .add(Restrictions.eq("name", domainName))
                .uniqueResult());
        persister.create(new Volume(domain, volumeName, volumePolicy.getMaxObjectSizeInBytes()));
    }

    @Override
    public void deleteVolume(String domainName, String volumeName) throws ApiException, TException {
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
    public VolumeDescriptor statVolume(String domainName, String volumeName) throws ApiException, TException {
        Volume volume = getVolume(domainName, volumeName);
        return new VolumeDescriptor(volumeName, volume.getTimestamp(),
                                    new VolumePolicy(volume.getObjectSize(),
                                                     VolumeConnector.S3));
    }

    @Override
    public List<VolumeDescriptor> listVolumes(String domainName) throws ApiException, TException {
        List<Volume> volumes = persister.execute(session ->
                session.createCriteria(Volume.class)
                        .createCriteria("domain")
                        .add(Restrictions.eq("name", domainName))
                        .list());

        return volumes.stream()
                .map(v -> new VolumeDescriptor(v.getName(), v.getTimestamp(),
                                               new VolumePolicy(v.getObjectSize(),
                                                                VolumeConnector.S3)))
                .collect(Collectors.toList());
    }


    @Override
    public List<BlobDescriptor> volumeContents(String domainName, String volumeName, int count, long offset) throws ApiException, TException {
        List<Blob> blobs = getAllVolumeBlobs(domainName, volumeName);

        return blobs.stream()
                .skip(offset)
                .limit(count)
                .map(b -> new BlobDescriptor(b.getName(), b.getByteCount(), ByteBuffer.wrap(new byte[0]), b.getMetadata()))
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
    public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
        Blob blob = getBlob(domainName, volumeName, blobName);
        if (blob == null) {
            throw new ApiException("No such resource", ErrorCode.MISSING_RESOURCE);
        }
        return new BlobDescriptor(blobName, blob.getByteCount(), ByteBuffer.wrap(blob.getDigest()), blob.getMetadata());
    }

    @Override
    public TxDescriptor startBlobTx(String domainName, String volumeName, String blobName) throws ApiException, TException {        
        return new TxDescriptor(0);
    }

    @Override
    public void commitBlobTx(TxDescriptor txDesc) throws ApiException, TException {
    }

    @Override
    public void abortBlobTx(TxDescriptor txDesc) throws ApiException, TException {
    }

    @Override
    public void attachVolume(String domainName, String volumeName) throws ApiException, TException {
    }

    @Override
    public VolumeStatus volumeStatus(String domainName, String volumeName) throws ApiException, TException {
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
    public ByteBuffer getBlob(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) throws ApiException, TException {
        Blob blob = getBlob(domainName, volumeName, blobName);
        int objectSize = blob.getVolume().getObjectSize();

        Function<Integer, byte[]> provider = i -> {
            return getOrMakeBlock(i, blob.getId(), objectSize).getBytes();
        };

        byte[] read = new BlockReader().read(provider, objectSize, offset.getValue() * objectSize, length);
        return ByteBuffer.wrap(read);
    }

    @Override
    public void updateMetadata(String domainName, String volumeName, String blobName, TxDescriptor txDesc, Map<String, String> metadata) throws ApiException, TException {
        Blob blob = getOrCreate(domainName, volumeName, blobName);
        blob.setMetadataJson(new JSONObject(metadata).toString(2));
        persister.update(blob);
    }

    @Override
    public void updateBlob(String domainName, String volumeName, String blobName, TxDescriptor txDesc, ByteBuffer bytes, int length, ObjectOffset objectOffset, ByteBuffer digest, boolean isLast) throws ApiException, TException {
        Blob blob = getOrCreate(domainName, volumeName, blobName);
        int objectSize = blob.getVolume().getObjectSize();
        long newByteCount = Math.max(blob.getByteCount(), objectSize * objectOffset.getValue() + length);
        int blockSize = blob.getVolume().getObjectSize();
        blob.setByteCount(newByteCount);
        blob.setDigest(digest.array());
        BlockWriter writer = new BlockWriter(i -> getOrMakeBlock(i, blob.getId(), blockSize), blockSize);
        Iterator<Block> updated = writer.update(bytes.array(), length, objectOffset.getValue() * objectSize);
        while (updated.hasNext()) {
            Block next = updated.next();
            if (next.getId() == -1) {
                persister.create(next);
            } else {
                persister.update(next);
            }
        }
        persister.update(blob);
    }

    @Override
    public void deleteBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
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
            blob = persister.create(new Blob(getVolume(domainName, volumeName), blobName, new byte[0]));
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

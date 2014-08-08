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
import java.util.ArrayList;

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
    public void createVolume(String domainName, String volumeName, VolumeSettings volumePolicy) throws ApiException, TException {
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
                                    new VolumeSettings(volume.getObjectSize(),
                                                     VolumeType.OBJECT, 0));
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
                                               new VolumeSettings(v.getObjectSize(),
                                                                VolumeType.OBJECT, 0)))
                .collect(Collectors.toList());
    }

    @Override
    public com.formationds.streaming.registration registerStream(String url, String http_method, List<String> volume_names, int sample_freq_seconds, int duration_seconds) throws org.apache.thrift.TException {
        com.formationds.streaming.registration reg = new com.formationds.streaming.registration();
        return reg;
    }

    @Override
    public List<com.formationds.streaming.registration> getStreamRegistrations(int count) throws org.apache.thrift.TException {
        List<com.formationds.streaming.registration> regList = new ArrayList<com.formationds.streaming.registration>();
        return regList;
    }

    @Override
    public void deregisterStream(int registration_id) throws org.apache.thrift.TException {
        
    }



    @Override
    public List<BlobDescriptor> volumeContents(String domainName, String volumeName, int count, long offset) throws ApiException, TException {
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
    public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
        Blob blob = getBlob(domainName, volumeName, blobName);
        if (blob == null) {
            throw new ApiException("No such resource", ErrorCode.MISSING_RESOURCE);
        }
        return new BlobDescriptor(blobName, blob.getByteCount(), blob.getMetadata());
    }

    @Override
    public TxDescriptor startBlobTx(String domainName, String volumeName, String blobName, int blobMode) throws ApiException, TException {
        return new TxDescriptor(0);
    }

    @Override
    public void  commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDesc) throws ApiException, TException {
    }

    @Override
    public void abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDesc) throws ApiException, TException {        
    }

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
        return new VolumeStatus();
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
    public void updateBlob(String domainName, String volumeName, String blobName, TxDescriptor txDesc, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) throws ApiException, TException {
        Blob blob = getOrCreate(domainName, volumeName, blobName);
        int objectSize = blob.getVolume().getObjectSize();
        long newByteCount = Math.max(blob.getByteCount(), objectSize * objectOffset.getValue() + length);
        int blockSize = blob.getVolume().getObjectSize();
        blob.setByteCount(newByteCount);
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
    public void updateBlobOnce(String domainName, String volumeName, String blobName, int blobMode, ByteBuffer bytes, int length, ObjectOffset objectOffset, Map<String, String> metadata) throws ApiException, TException {

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

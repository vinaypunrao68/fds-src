package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.AmShim;
import com.formationds.xdi.BlobDescriptor;
import com.formationds.xdi.FdsException;
import com.formationds.xdi.VolumeDescriptor;
import org.apache.thrift.TException;
import org.hibernate.criterion.Restrictions;

import java.nio.ByteBuffer;
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
    public void createVolume(String domainName, String volumeName, VolumeDescriptor volumeDescriptor) throws FdsException, TException {
        Domain domain = (Domain) persister.execute(session -> session.createCriteria(Domain.class)
                .add(Restrictions.eq("name", domainName))
                .uniqueResult());
        persister.create(new Volume(domain, volumeName, volumeDescriptor.getObjectSizeInBytes()));
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
        return new VolumeDescriptor(volume.getObjectSize());
    }

    @Override
    public List<String> listVolumes(String domainName) throws FdsException, TException {
        List<Volume> volumes = persister.execute(session ->
                session.createCriteria(Volume.class)
                        .createCriteria("domain")
                        .add(Restrictions.eq("name", domainName))
                        .list());

        return volumes.stream()
                .map(v -> v.getName())
                .collect(Collectors.toList());
    }

    @Override
    public List<String> volumeContents(String domainName, String volumeName) throws FdsException, TException {
        return null;
    }

    @Override
    public BlobDescriptor statBlob(String domainName, String volumeName) throws FdsException, TException {
        return null;
    }

    @Override
    public ByteBuffer getBlob(String domainName, String volumeName, String blobName, int length, long offset) throws FdsException, TException {
        return null;
    }

    @Override
    public void updateMetadata(String domainName, String volumeName, String blobName, Map<String, String> metadata) throws FdsException, TException {

    }

    @Override
    public void updateBlob(String domainName, String volumeName, String blobName, ByteBuffer bytes, int length, long offset) throws FdsException, TException {

    }
}

package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.*;
import org.apache.thrift.TException;

import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class Xdi implements AmShim.Iface {
    private final AmShim.Iface am;

    public Xdi(AmShim.Iface am) {
        this.am = am;
    }

    public void createVolume(String domainName, String volumeName, VolumePolicy volumePolicy) throws XdiException, TException {
        am.createVolume(domainName, volumeName, volumePolicy);
    }

    public void deleteVolume(String domainName, String volumeName) throws XdiException, TException {
        am.deleteVolume(domainName, volumeName);
    }

    public VolumeDescriptor statVolume(String domainName, String volumeName) throws XdiException, TException {
        return am.statVolume(domainName, volumeName);
    }

    public List<VolumeDescriptor> listVolumes(String domainName) throws XdiException, TException {
        return am.listVolumes(domainName);
    }

    public List<BlobDescriptor> volumeContents(String domainName, String volumeName, int count, long offset) throws XdiException, TException {
        return am.volumeContents(domainName, volumeName, count, offset);
    }

    public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws XdiException, TException {
        return am.statBlob(domainName, volumeName, blobName);
    }

    public InputStream readStream(String domainName, String volumeName, String blobName) throws Exception {
        Iterator<byte[]> iterator = new BlockIterator(this).read(domainName, volumeName, blobName);
        return new BlockStreamer(iterator);
    }

    public void writeStream(String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        VolumeDescriptor volume = statVolume(domainName, volumeName);
        int bufSize = volume.getPolicy().getObjectSizeInBytes();
        new StreamWriter(bufSize, this).write(domainName, volumeName, blobName, in, metadata);
    }

    public Uuid startBlobTx(String domainName, String volumeName, String blobName) throws XdiException, TException {
        return am.startBlobTx(domainName, volumeName, blobName);
    }

    public void commit(Uuid txId) throws XdiException, TException {
        am.commit(txId);
    }

    public VolumeStatus volumeStatus(String domainName, String volumeName) throws XdiException, TException {
        return am.volumeStatus(domainName, volumeName);
    }

    public ByteBuffer getBlob(String domainName, String volumeName, String blobName, int length, long offset) throws XdiException, TException {
        return am.getBlob(domainName, volumeName, blobName, length, offset);
    }

    public void updateMetadata(String domainName, String volumeName, String blobName, Uuid txUuid, Map<String, String> metadata) throws XdiException, TException {
        am.updateMetadata(domainName, volumeName, blobName, txUuid, metadata);
    }

    public void updateBlob(String domainName, String volumeName, String blobName, Uuid txUuid, ByteBuffer bytes, int length, long offset) throws XdiException, TException {
        am.updateBlob(domainName, volumeName, blobName, txUuid, bytes, length, offset);
    }

    public void deleteBlob(String domainName, String volumeName, String blobName) throws XdiException, TException {
        am.deleteBlob(domainName, volumeName, blobName);
    }
}

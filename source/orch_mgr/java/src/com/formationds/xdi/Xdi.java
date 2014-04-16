package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.*;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;

public class Xdi implements AmShim.Iface {
    private final AmShim.Iface am;

    public Xdi(AmShim.Iface am) {
        this.am = am;
    }

    public void createVolume(String domainName, String volumeName, VolumePolicy volumePolicy) throws FdsException, TException {
        am.createVolume(domainName, volumeName, volumePolicy);
    }

    public void deleteVolume(String domainName, String volumeName) throws FdsException, TException {
        am.deleteVolume(domainName, volumeName);
    }

    public VolumeDescriptor statVolume(String domainName, String volumeName) throws FdsException, TException {
        return am.statVolume(domainName, volumeName);
    }

    public List<VolumeDescriptor> listVolumes(String domainName) throws FdsException, TException {
        return am.listVolumes(domainName);
    }

    public List<BlobDescriptor> volumeContents(String domainName, String volumeName, int count, long offset) throws FdsException, TException {
        return am.volumeContents(domainName, volumeName, count, offset);
    }

    public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws FdsException, TException {
        return am.statBlob(domainName, volumeName, blobName);
    }

    public Uuid startBlobTx(String domainName, String volumeName, String blobName) throws FdsException, TException {
        return am.startBlobTx(domainName, volumeName, blobName);
    }

    public void commit(Uuid txId) throws FdsException, TException {
        am.commit(txId);
    }

    public VolumeStatus volumeStatus(String domainName, String volumeName) throws FdsException, TException {
        return am.volumeStatus(domainName, volumeName);
    }

    public ByteBuffer getBlob(String domainName, String volumeName, String blobName, int length, long offset) throws FdsException, TException {
        return am.getBlob(domainName, volumeName, blobName, length, offset);
    }

    public void updateMetadata(String domainName, String volumeName, String blobName, Uuid txUuid, Map<String, String> metadata) throws FdsException, TException {
        am.updateMetadata(domainName, volumeName, blobName, txUuid, metadata);
    }

    public void updateBlob(String domainName, String volumeName, String blobName, Uuid txUuid, ByteBuffer bytes, int length, long offset) throws FdsException, TException {
        am.updateBlob(domainName, volumeName, blobName, txUuid, bytes, length, offset);
    }

    public void deleteBlob(String domainName, String volumeName, String blobName) throws FdsException, TException {
        am.deleteBlob(domainName, volumeName, blobName);
    }
}

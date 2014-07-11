package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.List;
import java.util.Map;

public class FakeAmService implements AmService.Iface {
    @Override
    public void attachVolume(String domainName, String volumeName) throws ApiException, TException {

    }

    @Override
    public List<BlobDescriptor> volumeContents(String domainName, String volumeName, int count, long offset) throws ApiException, TException {
        return Collections.emptyList();
    }

    @Override
    public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
        throw new ApiException("not implemented", ErrorCode.MISSING_RESOURCE);
    }

    @Override
    public TxDescriptor startBlobTx(String domainName, String volumeName, String blobName) throws ApiException, TException {
        return new TxDescriptor();
    }

    @Override
    public void commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDesc) throws ApiException, TException {
    }

    @Override
    public void abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDesc) throws ApiException, TException {
    }

    @Override
    public ByteBuffer getBlob(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) throws ApiException, TException {
        return ByteBuffer.wrap(new byte[0]);
    }

    @Override
    public void updateMetadata(String domainName, String volumeName, String blobName, TxDescriptor txDesc, Map<String, String> metadata) throws ApiException, TException {

    }

    @Override
    public void updateBlob(String domainName, String volumeName, String blobName, TxDescriptor txDesc, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) throws ApiException, TException {

    }

    @Override
    public void deleteBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {

    }

    @Override
    public VolumeStatus volumeStatus(String domainName, String volumeName) throws ApiException, TException {
        return new VolumeStatus();
    }
}

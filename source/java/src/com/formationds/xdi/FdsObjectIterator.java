package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.VolumeDescriptor;
import org.apache.thrift.TException;

import java.util.Iterator;

public class FdsObjectIterator {
    private AmService.Iface am;
    private ConfigurationService.Iface config;

    public FdsObjectIterator(AmService.Iface am, ConfigurationService.Iface config) {
        this.am = am;
        this.config = config;
    }

    public Iterator<byte[]> read(String domainName, String volumeName, String blobName) throws Exception {
        VolumeDescriptor volumeDescriptor = config.statVolume(domainName, volumeName);
        int fdsObjectSize = volumeDescriptor.getPolicy().getMaxObjectSizeInBytes();
        long byteCount = am.statBlob(domainName, volumeName, blobName).getByteCount();
        return read(domainName, volumeName, blobName, 0, byteCount, fdsObjectSize);
    }

    public Iterator<byte[]> read(String domainName, String volumeName, String blobName, long startOffset, long length) throws Exception {
        VolumeDescriptor volumeDescriptor = config.statVolume(domainName, volumeName);
        int fdsObjectSize = volumeDescriptor.getPolicy().getMaxObjectSizeInBytes();
        return read(domainName, volumeName, blobName, startOffset, length, fdsObjectSize);
    }

    private Iterator<byte[]> read(String domainName, String volumeName, String blobName, long requestBytesOffset, long requestBytesLength, long fdsObjectSize) {

        long totalObjects = (long) Math.ceil(requestBytesLength / (double) fdsObjectSize) + Math.min(1, requestBytesOffset % fdsObjectSize);

        return new Iterator<byte[]>() {

            long numObjectsRead = 0;

            @Override
            public boolean hasNext() {
                return numObjectsRead < totalObjects;
            }

            @Override
            public byte[] next() {
                long startObjectOffset = requestBytesOffset / fdsObjectSize;
                ObjectOffset objectOffset = new ObjectOffset(startObjectOffset + numObjectsRead);
                long readLength = fdsObjectSize;
                // For the last object, only read as many bytes as we care about
                if(numObjectsRead == totalObjects - 1)
                    readLength = (requestBytesLength - requestBytesOffset) % fdsObjectSize;

                byte[] object = null;
                try {
                    object = am.getBlob(domainName, volumeName, blobName, (int) readLength, new ObjectOffset(objectOffset)).array();
                } catch (TException e) {
                    throw new RuntimeException(e);
                }

                numObjectsRead++;

                // For the first object, truncate the first part if it does not lie on an object boundary
                if(objectOffset.getValue() == startObjectOffset && requestBytesOffset % fdsObjectSize != 0) {
                    long chunkOffset = requestBytesOffset % fdsObjectSize;
                    long chunkLength = fdsObjectSize - chunkOffset;

                    byte[] objectChunk = new byte[(int)chunkLength];
                    System.arraycopy(object, (int)chunkOffset, objectChunk, 0, (int)chunkLength);
                    return objectChunk;
                }

                return  object;
            }
        };
    }
}

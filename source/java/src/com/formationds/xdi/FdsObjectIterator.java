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

    private Iterator<byte[]> read(String domainName, String volumeName, String blobName, long requestBytesOffset, long requestBytesLength, int fdsObjectSize) {
        FdsObjectFrame.FrameIterator frameIterator = new FdsObjectFrame.FrameIterator(requestBytesOffset, requestBytesLength, fdsObjectSize);
        return new Iterator<byte[]>() {
            @Override
            public boolean hasNext() {
                return frameIterator.hasNext();
            }

            @Override
            public byte[] next() {
                FdsObjectFrame frame = frameIterator.next();
                try {
                    if(frame.internalOffset != 0) {
                        int readLength = frame.internalOffset + frame.internalLength;
                        byte[] object = am.getBlob(domainName, volumeName, blobName, readLength, new ObjectOffset(frame.objectOffset)).array();
                        byte[] result = new byte[frame.internalLength];
                        System.arraycopy(object, frame.internalOffset, result, 0, frame.internalLength);
                        return result;
                    } else {
                        return am.getBlob(domainName, volumeName, blobName, frame.internalLength, new ObjectOffset(frame.objectOffset)).array();
                    }
                } catch (TException e) {
                    throw new RuntimeException(e);
                }
            }
        };
    }
}

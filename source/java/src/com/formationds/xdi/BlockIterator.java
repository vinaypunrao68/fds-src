package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.ObjectOffset;
import com.formationds.xdi.shim.VolumeDescriptor;
import org.apache.thrift.TException;

import java.util.Iterator;

public class BlockIterator {
    private AmShim.Iface am;

    public BlockIterator(AmShim.Iface am) {
        this.am = am;
    }

    public Iterator<byte[]> read(String domainName, String volumeName, String blobName) throws Exception {
        VolumeDescriptor volumeDescriptor = am.statVolume(domainName, volumeName);
        int blockSize = volumeDescriptor.getPolicy().getMaxObjectSizeInBytes();
        long byteCount = am.statBlob(domainName, volumeName, blobName).getByteCount();
        long totalObjects = (long) Math.ceil(byteCount / (double) blockSize);

        return new Iterator<byte[]>() {
            long currentObjectOffset = 0;
            byte[] currentChunk = null;
            boolean isDirty = true;

            private void refreshIfNeeded() {
                if (isDirty) {
                    if (currentObjectOffset == totalObjects) {
                        currentChunk = null;
                        return;
                    }

                    long length = Math.min(byteCount - (currentObjectOffset * blockSize), blockSize);
                    try {
                        currentChunk = am.getBlob(domainName, volumeName, blobName, (int) length, new ObjectOffset(currentObjectOffset)).array();
                    } catch (TException e) {
                        throw new RuntimeException(e);
                    }
                    currentObjectOffset++;
                    isDirty = false;
                }
            }

            @Override
            public boolean hasNext() {
                refreshIfNeeded();
                return currentChunk != null;
            }

            @Override
            public byte[] next() {
                refreshIfNeeded();
                isDirty = true;
                return currentChunk;
            }
        };
    }
}

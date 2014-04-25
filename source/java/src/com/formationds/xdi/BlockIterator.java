package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.VolumeDescriptor;

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

        return new Iterator<byte[]>() {
            byte[] current = null;
            long currentOffset = 0;
            boolean isDirty = true;

            private void refreshIfNeeded() {
                if (isDirty) {
                    if (currentOffset < byteCount) {
                        long chunkSize = Math.min(blockSize, byteCount - currentOffset);
                        try {
                            current = am.getBlob(domainName, volumeName, blobName, (int) chunkSize, currentOffset).array();
                            currentOffset += chunkSize;
                        } catch (Exception e) {
                            throw new RuntimeException(e);
                        }
                        isDirty = false;
                    } else {
                        current = null;
                    }
                }
            }
            @Override
            public boolean hasNext() {
                refreshIfNeeded();
                return current != null;
            }

            @Override
            public byte[] next() {
                refreshIfNeeded();
                isDirty = true;
                return current;
            }
        };
    }
}

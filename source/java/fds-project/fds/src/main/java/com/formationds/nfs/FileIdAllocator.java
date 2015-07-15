package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.xdi.AsyncAm;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class FileIdAllocator {
    public static final String NFS_MAX_FILE_ID = "NFS_MAX_FILE_ID";
    public static final int START_VALUE = Integer.MAX_VALUE;
    public static final int EXPORT_ROOT_VALUE = Integer.MAX_VALUE - 1;

    private AsyncAm asyncAm;
    private ConcurrentHashMap<String, Object> volumeLocks;

    public FileIdAllocator(AsyncAm asyncAm) {
        this.asyncAm = asyncAm;
        volumeLocks = new ConcurrentHashMap<>();
    }

    // TODO: the allocation lock could be striped, perhaps using the NfsPath as a striping key
    public long nextId(String volume) throws IOException {
        Object volumeLock = volumeLocks.computeIfAbsent(volume, v -> new Object());
        synchronized (volumeLock) {
            try {
                String blobName = new NfsPath(volume, "/").blobName();
                BlobDescriptor blobDescriptor = asyncAm.statBlob(AmVfs.DOMAIN, volume, blobName).get();
                Map<String, String> blobMetadata = blobDescriptor.getMetadata();
                long currentValue = Long.parseLong(blobMetadata.getOrDefault(NFS_MAX_FILE_ID, Integer.toString(START_VALUE)));
                long newValue = currentValue + 1;
                blobMetadata.put(NFS_MAX_FILE_ID, Long.toString(newValue));
                asyncAm.updateBlobOnce(AmVfs.DOMAIN, volume, blobName, 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), blobMetadata).get();
                return newValue;
            } catch (Exception e) {
                throw new IOException(e);
            }
        }
    }
}

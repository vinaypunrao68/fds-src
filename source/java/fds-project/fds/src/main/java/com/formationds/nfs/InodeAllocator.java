package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import org.apache.log4j.Logger;
import org.dcache.nfs.vfs.Inode;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class InodeAllocator {
    private static final Logger LOG = Logger.getLogger(InodeAllocator.class);
    private AsyncAm asyncAm;
    private Cache<String, Object> locks;
    private ExportResolver exportResolver;
    private static final String NUMBER_WELL = "number-well";

    public InodeAllocator(AsyncAm asyncAm, ExportResolver exportResolver) {
        this.asyncAm = asyncAm;
        this.exportResolver = exportResolver;
        locks = CacheBuilder.newBuilder()
                .expireAfterAccess(1, TimeUnit.MINUTES)
                .build();
    }

    public long allocate(String volume) throws IOException {
        Object lock = null;
        try {
            lock = locks.get(volume, () -> new Object());
        } catch (ExecutionException e) {
            throw new IOException(e.getCause());
        }
        long volumeId = exportResolver.exportId(volume);
        synchronized (lock) {
            long[] currentValue = new long[]{volumeId};
            try {
                currentValue[0] = Long.parseLong(unwindExceptions(() -> asyncAm.statBlob(BlockyVfs.DOMAIN, volume, NUMBER_WELL).get())
                        .getMetadata()
                        .get(NUMBER_WELL));
            } catch (ApiException e) {
                if (!e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                    throw new IOException(e);
                }
            } catch (Exception e) {
                throw new IOException(e);
            }
            currentValue[0] += 1;
            Map<String, String> meta = new HashMap<>();
            meta.put(NUMBER_WELL, Long.toString(currentValue[0]));
            try {
                unwindExceptions(() -> asyncAm.updateBlobOnce(BlockyVfs.DOMAIN, volume, NUMBER_WELL, 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), meta).get());
            } catch (Exception e) {
                throw new IOException(e);
            }
            return currentValue[0];
        }
    }

    public static boolean isExportRoot(Inode inode) {
        long fileId = InodeMetadata.fileId(inode);
        int exportId = inode.exportIndex();
        return fileId == exportId;
    }
}

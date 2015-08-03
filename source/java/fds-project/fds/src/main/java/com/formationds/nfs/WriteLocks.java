package com.formationds.nfs;

import com.formationds.commons.util.SupplierWithExceptions;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import org.dcache.nfs.vfs.Inode;

import java.io.IOException;
import java.util.concurrent.ExecutionException;

public class WriteLocks {

    private final Cache<Key, Object> cache;

    public WriteLocks() {
        cache = CacheBuilder.newBuilder()
                .maximumSize(1000)
                .build();
    }

    public <T> T guard(Inode inode, SupplierWithExceptions<T> supplier) throws IOException {
        Object lockObj = null;
        try {
            lockObj = cache.get(new Key(inode), () -> new Object());
        } catch (ExecutionException e) {
            throw new IOException(e);
        }
        synchronized (lockObj) {
            try {
                return supplier.supply();
            } catch (Exception e) {
                throw new IOException(e);
            }
        }
    }

    private static class Key {
        long exportId;
        long fileId;

        public Key(Inode inode) {
            exportId = inode.exportIndex();
            fileId = InodeMetadata.fileId(inode);
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Key key = (Key) o;

            if (exportId != key.exportId) return false;
            if (fileId != key.fileId) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = (int) (exportId ^ (exportId >>> 32));
            result = 31 * result + (int) (fileId ^ (fileId >>> 32));
            return result;
        }
    }
}

package com.formationds.nfs;

import org.dcache.nfs.status.NoSpcException;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;
import org.junit.Test;

import javax.security.auth.Subject;

import static org.junit.Assert.fail;

public class InodeMapTest {
    private static final String VOLUME = "foo";

    @Test
    public void testMaxCapacityIsEnforced() throws Exception {
        long maxVolumeCapacityInBytes = 100;
        StubExportResolver exportResolver = new StubExportResolver(VOLUME, 1024, maxVolumeCapacityInBytes);
        InodeMap inodeMap = new InodeMap(new MemoryIoOps(), exportResolver);
        Inode inode = inodeMap.create(new InodeMetadata(Stat.Type.REGULAR, new Subject(), 0644, 257), exportResolver.exportId(VOLUME), true);
        inodeMap.write(inode, new byte[100], 0, 100);

        try {
            inodeMap.write(inode, new byte[100], 100, 100);
        } catch (NoSpcException e) {
            return;
        }

        fail("Should have gotten a NoSpcException!");
    }

}
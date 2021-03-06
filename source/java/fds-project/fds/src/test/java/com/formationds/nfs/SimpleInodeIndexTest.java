package com.formationds.nfs;

import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.Stat;
import org.junit.Test;

import javax.security.auth.Subject;
import java.util.List;
import java.util.Optional;

import static org.junit.Assert.assertEquals;

public class SimpleInodeIndexTest {
    public static final int MAX_OBJECT_SIZE = 8;
    public static final String VOLUME = "volume";
    public static final String SUE = "sue";
    public static final String JOHN = "john";

    @Test
    public void testIndex() throws Exception {
        ExportResolver exportResolver = new StubExportResolver(VOLUME, MAX_OBJECT_SIZE, Long.MAX_VALUE);
        IoOps io = new MemoryIoOps();
        SimpleInodeIndex index = new SimpleInodeIndex(io, exportResolver);
        int exportId = exportResolver.nfsExportId(VOLUME);
        int parentId = 3;
        int sueId = 4;
        int johnId = 5;
        InodeMetadata parent = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0, parentId);
        InodeMetadata daughter = new InodeMetadata(Stat.Type.REGULAR, new Subject(), 0, sueId).withLink(parentId, SUE);
        InodeMetadata son = new InodeMetadata(Stat.Type.REGULAR, new Subject(), 0, johnId).withLink(parentId, JOHN);
        index.index(StubExportResolver.EXPORT_ID, true, parent);
        index.index(StubExportResolver.EXPORT_ID, true, daughter);
        index.index(StubExportResolver.EXPORT_ID, true, son);
        Optional<InodeMetadata> result = index.lookup(parent.asInode(StubExportResolver.EXPORT_ID), SUE);
        assertEquals(sueId, result.get().getFileId());
        List<DirectoryEntry> children = index.list(parent, StubExportResolver.EXPORT_ID);
        assertEquals(2, children.size());
    }
}

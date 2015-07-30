package com.formationds.nfs;

import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.Stat;
import org.junit.Test;

import javax.security.auth.Subject;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

public class InodeIndexTest {
    @Test
    public void testUpdate() throws Exception {
        InodeIndex index = new InodeIndex(null);
        int volumeId = 1;
        InodeMetadata dir = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0, 3, volumeId);
        InodeMetadata child = new InodeMetadata(Stat.Type.REGULAR, new Subject(), 0, 4, volumeId)
                .withLink(dir.getFileId(), "panda");

        index.index(volumeId, dir, child);
        child = child.withUpdatedAtime();
        index.index(volumeId, child);
        List<DirectoryEntry> list = index.list(dir.asInode());
        assertEquals(1, list.size());
    }

    @Test
    public void testLookup() throws Exception {
        InodeIndex index = new InodeIndex(null);
        int volumeId = 1;
        InodeMetadata fooDir = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0, 2, volumeId);
        InodeMetadata barDir = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0, 3, volumeId);

        InodeMetadata child = new InodeMetadata(Stat.Type.REGULAR, new Subject(), 0, 4, volumeId)
                .withLink(fooDir.getFileId(), "panda")
                .withLink(barDir.getFileId(), "lemur");

        index.index(volumeId, fooDir, child);
        assertEquals(child, index.lookup(fooDir.asInode(), "panda").get());
        assertEquals(child, index.lookup(barDir.asInode(), "lemur").get());
        assertFalse(index.lookup(fooDir.asInode(), "baboon").isPresent());
        assertFalse(index.lookup(barDir.asInode(), "giraffe").isPresent());
        index.remove(child.asInode());
        assertFalse(index.lookup(fooDir.asInode(), "panda").isPresent());
        assertFalse(index.lookup(barDir.asInode(), "lemur").isPresent());
    }

    @Test
    public void testListDirectory() throws Exception {
        InodeIndex index = new InodeIndex(null);
        int volumeId = 1;
        InodeMetadata fooDir = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0, 1, volumeId);
        InodeMetadata barDir = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0, 2, volumeId);

        InodeMetadata blue = new InodeMetadata(Stat.Type.REGULAR, new Subject(), 0, 3, volumeId)
                .withLink(fooDir.getFileId(), "blue")
                .withLink(barDir.getFileId(), "blue");

        InodeMetadata red = new InodeMetadata(Stat.Type.REGULAR, new Subject(), 0, 4, volumeId)
                .withLink(fooDir.getFileId(), "red");

        index.index(volumeId, fooDir, barDir, blue, red);
        assertEquals(2, index.list(fooDir.asInode()).size());
        assertEquals(1, index.list(barDir.asInode()).size());
        assertEquals(0, index.list(blue.asInode()).size());
    }
}
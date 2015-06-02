package com.formationds.nfs;

import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;
import org.junit.Test;

import static org.junit.Assert.*;

public class NfsPathTest {
    @Test
    public void testFromInode() throws Exception {
        NfsPath nfsPath = new NfsPath(new Inode(new FileHandle(0, 0, 0, "/foo/bar/".getBytes())));
        assertEquals("foo", nfsPath.getVolume());
        assertEquals("/bar", nfsPath.blobName());
        assertFalse(nfsPath.isRoot());
    }

    @Test
    public void testVolumeOnly() throws Exception {
        NfsPath nfsPath = new NfsPath(new Inode(new FileHandle(0, 0, 0, "/foo".getBytes())));
        assertEquals("foo", nfsPath.getVolume());
        assertEquals("/", nfsPath.blobName());
        assertFalse(nfsPath.isRoot());
    }

    @Test
    public void testFromParent() throws Exception {
        NfsPath parent = new NfsPath(new Inode(new FileHandle(0, 0, 0, "/panda/foo".getBytes())));
        NfsPath child = new NfsPath(parent, "bar");
        assertEquals("/foo/bar", child.blobName());
        assertEquals("bar", child.fileName());
    }

    @Test
    public void testAsInode() throws Exception {
        NfsPath parent = new NfsPath(new Inode(new FileHandle(0, 0, 0, "/panda/foo".getBytes())));
        NfsPath child = new NfsPath(parent, "bar");

        Inode childInode = child.asInode(Stat.Type.REGULAR);
        NfsPath thawed = new NfsPath(childInode);
        assertEquals(child, thawed);
    }

    @Test
    public void testRootInode() throws Exception {
        NfsPath root = new NfsPath(new Inode(new FileHandle(0, 0, Stat.S_IFDIR, new byte[0])));
        assertNull(root.getVolume());
        assertEquals("/", root.blobName());
        assertTrue(root.isRoot());
        NfsPath child = new NfsPath(root, "foo");
        assertEquals("foo", child.getVolume());
        assertEquals("/", child.blobName());
    }
}
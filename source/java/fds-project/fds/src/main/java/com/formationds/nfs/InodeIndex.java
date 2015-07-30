package com.formationds.nfs;

import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.Inode;

import java.io.IOException;
import java.util.List;
import java.util.Optional;

public interface InodeIndex {
    Optional<InodeMetadata> lookup(Inode parent, String name) throws IOException;

    List<DirectoryEntry> list(Inode inode) throws IOException;

    void index(InodeMetadata... entries) throws IOException;

    void remove(InodeMetadata inodeMetadata) throws IOException;

    void unlink(Inode parentInode, String path) throws IOException;
}

package com.formationds.nfs;

import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.Inode;

import java.io.IOException;
import java.util.List;
import java.util.Optional;

public interface InodeIndex {
    Optional<InodeMetadata> lookup(Inode parent, String name) throws IOException;

    List<DirectoryEntry> list(InodeMetadata inodeMetadata, long exportId) throws IOException;

    void index(long exportId, boolean deferrable, InodeMetadata entry) throws IOException;

    void remove(long exportId, InodeMetadata inodeMetadata) throws IOException;

    void unlink(long exportId, long parentId, String linkName) throws IOException;
}

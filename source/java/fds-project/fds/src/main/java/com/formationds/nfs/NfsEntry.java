package com.formationds.nfs;

import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;

public class NfsEntry {
    private NfsPath path;
    private NfsAttributes attributes;

    NfsEntry(NfsPath path, NfsAttributes attributes) {
        this.path = path;
        this.attributes = attributes;
    }

    public NfsPath path() {
        return path;
    }

    public NfsAttributes attributes() {
        return attributes;
    }

    public NfsEntry withUpdatedMtime() {
        attributes = attributes.withUpdatedMtime();
        return this;
    }

    public NfsEntry withUpdatedAtime() {
        attributes = attributes.withUpdatedAtime();
        return this;
    }

    public NfsEntry withUpdatedSize(long size) {
        attributes = attributes.withUpdatedSize(size);
        return this;
    }

    public boolean isDirectory() {
        return attributes.getType().equals(Stat.Type.DIRECTORY);
    }

    public Inode inode(ExportResolver resolver) {
        return path.asInode(attributes.getType(), resolver);
    }
}

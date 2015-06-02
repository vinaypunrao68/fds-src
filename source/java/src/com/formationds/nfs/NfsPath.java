package com.formationds.nfs;

import com.google.common.collect.Lists;
import org.apache.commons.io.FilenameUtils;
import org.apache.hadoop.fs.Path;
import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;

import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.StringTokenizer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class NfsPath {
    private String volume;
    private Path path;

    public static final Inode NFS_ROOT_INODE = new Inode(new FileHandle(0, 0, Stat.S_IFDIR, new byte[0]));

    public NfsPath(Inode inode) {
        if (inode.getFileId().length == 0) {
            this.path = new Path("/");
            return;
        }
        String complete = new String(inode.getFileId());
        StringTokenizer tokenizer = new StringTokenizer(complete, "/", false);

        volume = tokenizer.nextToken();
        StringBuilder sb = new StringBuilder();
        while (tokenizer.hasMoreTokens()) {
            sb.append("/");
            sb.append(tokenizer.nextToken());
        }
        if (sb.length() == 0) {
            sb.append("/");
        }
        path = new Path(sb.toString());
    }

    public NfsPath(String volume, String path) {
        this.volume = volume;
        this.path = new Path(path);
    }

    public NfsPath(NfsPath parent, String child) {
        if (parent.isRoot()) {
            volume = child;
            path = new Path("/");
            return;
        }

        this.volume = parent.getVolume();
        this.path = new Path(parent.getPath(), child);
    }

    public String getVolume() {
        return volume;
    }

    public Path getPath() {
        return path;
    }

    public boolean isRoot() {
        return volume == null && path.toString().equals("/");
    }

    public String blobName() {
        return path.toString();
    }

    public Inode asInode(Stat.Type type) {
        StringBuilder sb = new StringBuilder("/");
        if (volume != null) {
            sb.append(volume);
            sb.append(blobName());
        }
        return new Inode(new FileHandle(0, 0, type.toMode(), sb.toString().getBytes()));
    }

    public String fileName() {
        return path.getName();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        NfsPath nfsPath = (NfsPath) o;

        if (!path.equals(nfsPath.path)) return false;
        if (volume != null ? !volume.equals(nfsPath.volume) : nfsPath.volume != null) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = volume != null ? volume.hashCode() : 0;
        result = 31 * result + path.hashCode();
        return result;
    }

    @Override
    public String toString() {
        return "NfsPath{" +
                "volume='" + volume + '\'' +
                ", path=" + path +
                '}';
    }
}

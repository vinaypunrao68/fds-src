package com.formationds.nfs;

import org.apache.hadoop.fs.Path;
import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;

import java.util.StringTokenizer;

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

    public long deviceId(ExportResolver resolver) {
        if (isRoot()) {
            return Short.MAX_VALUE;
        } else {
            return resolver.exportId(volume) + 1 + Short.MAX_VALUE;
        }
    }

    public Inode asInode(Stat.Type type, ExportResolver resolver) {
        StringBuilder sb = new StringBuilder("/");
        int exportId = 0;
        if (volume != null) {
            sb.append(volume);
            sb.append(blobName());
            exportId = resolver.exportId(volume);
        }
        return new Inode(new FileHandle(0, exportId, type.toMode(), sb.toString().getBytes()));
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
        return "[" +
                "volume='" + volume + '\'' +
                ", path=" + path +
                ']';
    }
}

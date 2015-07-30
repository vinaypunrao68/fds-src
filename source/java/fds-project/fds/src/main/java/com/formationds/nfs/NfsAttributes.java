package com.formationds.nfs;

import com.formationds.protocol.BlobDescriptor;
import org.dcache.auth.Subjects;
import org.dcache.nfs.vfs.Stat;

import javax.security.auth.Subject;
import java.util.HashMap;
import java.util.Map;

public class NfsAttributes {
    public static final String NFS_TYPE = "NFS_TYPE";
    public static final String NFS_MODE = "NFS_MODE";
    public static final String NFS_FILE_ID = "NFS_FILE_ID";
    public static final String NFS_UID = "NFS_UID";
    public static final String NFS_GID = "NFS_GID";
    public static final String NFS_ATIME = "NFS_ATIME";
    public static final String NFS_CTIME = "NFS_CTIME";
    public static final String NFS_MTIME = "NFS_MTIME";
    public static final String NFS_SIZE = "NFS_SIZE";
    public static final String NFS_GENERATION = "NFS_GENERATION";
    public static final String NFS_NLINK = "NFS_NLINK";
    public static final String NFS_DEV = "NFS_DEV";
    private Map<String, String> metadata;

    public NfsAttributes(Stat.Type type, Subject subject, int mode, long fileId, long devId) {
        metadata = new HashMap<>();
        metadata.put(NFS_TYPE, type.name());
        int uid = (int) Subjects.getUid(subject);
        long[] gids = Subjects.getGids(subject);
        int gid = gids.length == 0? 0 : (int) gids[0];
        metadata.put(NFS_UID, Integer.toString(uid));
        metadata.put(NFS_GID, Integer.toString(gid));
        metadata.put(NFS_MODE, Integer.toString(mode));
        metadata.put(NFS_FILE_ID, Long.toString(fileId));
        long now = System.currentTimeMillis();
        metadata.put(NFS_ATIME, Long.toString(now));
        metadata.put(NFS_CTIME, Long.toString(now));
        metadata.put(NFS_MTIME, Long.toString(now));
        metadata.put(NFS_SIZE, Long.toString(0));
        metadata.put(NFS_GENERATION, Long.toString(0));
        metadata.put(NFS_NLINK, Long.toString(1));
        metadata.put(NFS_DEV, Long.toString(devId));
    }

    public NfsAttributes clone(long newFileId) {
        Map<String, String> cloned = new HashMap<>(metadata);
        cloned.put(NFS_FILE_ID, Long.toString(newFileId));
        return new NfsAttributes(cloned);
    }

    private NfsAttributes(Map<String, String> map) {
        this.metadata = map;
    }

    public NfsAttributes(BlobDescriptor blobDescriptor) {
        this.metadata = blobDescriptor.metadata;
    }

    public Map<String, String> asMetadata() {
        return metadata;
    }

    public NfsAttributes withIncrementedGeneration() {
        Map<String, String> metadata = new HashMap<>(this.metadata);
        long generation = Long.parseLong(metadata.get(NFS_GENERATION));
        metadata.put(NFS_GENERATION, Long.toString(generation + 1l));
        return new NfsAttributes(metadata);
    }

    public boolean isDirectory() {
        return getType().equals(Stat.Type.DIRECTORY);
    }

    public Stat asStat() {
        Stat stat = new Stat();
        Stat.Type type = getType();
        int mode = Integer.parseInt(metadata.get(NFS_MODE));
        stat.setMode(type.toMode() | mode);
        stat.setGeneration(Long.parseLong(metadata.get(NFS_GENERATION)));
        stat.setSize(Long.parseLong(metadata.get(NFS_SIZE)));
        stat.setFileid(Long.parseLong(metadata.get(NFS_FILE_ID)));
        stat.setNlink(1); // FIXME: implement linking
        stat.setUid(Integer.parseInt(metadata.get(NFS_UID)));
        stat.setGid(Integer.parseInt(metadata.get(NFS_GID)));
        stat.setATime(Long.parseLong(metadata.get(NFS_ATIME)));
        stat.setCTime(Long.parseLong(metadata.get(NFS_CTIME)));
        stat.setMTime(Long.parseLong(metadata.get(NFS_MTIME)));
        stat.setDev(Integer.parseInt(metadata.get(NFS_DEV)));
        return stat;
    }

    public Stat.Type getType() {
        return Enum.valueOf(Stat.Type.class, metadata.get(NFS_TYPE));
    }

    public NfsAttributes withUpdatedMtime() {
        return clone(NFS_MTIME, System.currentTimeMillis());
    }

    public NfsAttributes withUpdatedAtime() {
        return clone(NFS_ATIME, System.currentTimeMillis());
    }

    public NfsAttributes withUpdatedCtime() {
        return clone(NFS_CTIME, System.currentTimeMillis());
    }

    public NfsAttributes withUpdatedSize(long newSize) {
        return clone(NFS_SIZE, newSize);
    }

    private NfsAttributes clone(String fieldName, long newValue) {
        Map<String, String> metadata = new HashMap<>(this.metadata);
        metadata.put(fieldName, Long.toString(newValue));
        return new NfsAttributes(metadata);
    }

    public NfsAttributes withFileId(long fileId) {
        return clone(NFS_FILE_ID, fileId);
    }

    public long getSize() {
        return Long.parseLong(metadata.get(NFS_SIZE));
    }

    public NfsAttributes update(Stat stat) {
        if (stat.isDefined(Stat.StatAttribute.MODE)) {
            metadata.put(NFS_MODE, Integer.toString(stat.getMode()));
        }

        if (stat.isDefined(Stat.StatAttribute.SIZE)) {
            metadata.put(NFS_SIZE, Long.toString(stat.getSize()));
        }

        if (stat.isDefined(Stat.StatAttribute.OWNER)) {
            metadata.put(NFS_UID, Integer.toString(stat.getUid()));
        }

        if (stat.isDefined(Stat.StatAttribute.GROUP)) {
            metadata.put(NFS_GID, Integer.toString(stat.getGid()));
        }

        if (stat.isDefined(Stat.StatAttribute.ATIME)) {
            metadata.put(NFS_ATIME, Long.toString(stat.getATime()));
        }

        if (stat.isDefined(Stat.StatAttribute.CTIME)) {
            metadata.put(NFS_CTIME, Long.toString(stat.getCTime()));
        }

        if (stat.isDefined(Stat.StatAttribute.MTIME)) {
            metadata.put(NFS_MTIME, Long.toString(stat.getMTime()));
        }

        if (stat.isDefined(Stat.StatAttribute.DEV)) {
            metadata.put(NFS_DEV, Long.toString(stat.getDev()));
        }

        return this;
    }
}

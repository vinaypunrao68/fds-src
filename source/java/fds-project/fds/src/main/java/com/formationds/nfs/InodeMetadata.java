package com.formationds.nfs;

import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.Multimap;
import org.dcache.auth.Subjects;
import org.dcache.nfs.v4.xdr.*;
import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;
import org.json.JSONArray;
import org.json.JSONObject;

import javax.security.auth.Subject;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

public class InodeMetadata {
    public static final String NFS_FILE_ID = "NFS_FILE_ID";
    public static final String NFS_TYPE = "NFS_TYPE";
    public static final String NFS_MODE = "NFS_MODE";
    public static final String NFS_UID = "NFS_UID";
    public static final String NFS_GID = "NFS_GID";
    public static final String NFS_ATIME = "NFS_ATIME";
    public static final String NFS_CTIME = "NFS_CTIME";
    public static final String NFS_MTIME = "NFS_MTIME";
    public static final String NFS_SIZE = "NFS_SIZE";
    public static final String NFS_GENERATION = "NFS_GENERATION";
    public static final String NFS_LINKS = "NFS_LINKS";
    public static final String NFS_ACES = "NFS_ACES";

    public static final String ACE_MASK = "ace_mask";
    public static final String ACE_FLAG = "ace_flag";
    public static final String ACE_TYPE = "ace_type";
    public static final String ACE_WHO = "ace_who";

    public static final String PARENT = "parent";
    public static final String NAME = "name";

    private Map<String, String> metadata;

    public InodeMetadata(JSONObject o) {
        metadata = new HashMap<>();
        metadata.put(NFS_FILE_ID, Long.toString(o.getLong(NFS_FILE_ID)));
        metadata.put(NFS_TYPE, o.getString(NFS_TYPE));
        metadata.put(NFS_MODE, Integer.toString(o.getInt(NFS_MODE)));
        metadata.put(NFS_UID, Integer.toString(o.getInt(NFS_UID)));
        metadata.put(NFS_GID, Integer.toString(o.getInt(NFS_GID)));
        metadata.put(NFS_ATIME, Long.toString(o.getLong(NFS_ATIME)));
        metadata.put(NFS_CTIME, Long.toString(o.getLong(NFS_CTIME)));
        metadata.put(NFS_MTIME, Long.toString(o.getLong(NFS_MTIME)));
        metadata.put(NFS_SIZE, Long.toString(o.getLong(NFS_SIZE)));
        metadata.put(NFS_GENERATION, Long.toString(o.getLong(NFS_GENERATION)));
        metadata.put(NFS_LINKS, o.getJSONArray(NFS_LINKS).toString());
        metadata.put(NFS_ACES, o.has(NFS_ACES) ? o.getJSONArray(NFS_ACES).toString() : "[]");
    }

    public JSONObject asJsonObject() {
        JSONObject o = new JSONObject();
        o.put(NFS_FILE_ID, getFileId());
        o.put(NFS_TYPE, getType().name());
        o.put(NFS_MODE, getMode());
        o.put(NFS_UID, getUid());
        o.put(NFS_GID, getGid());
        o.put(NFS_ATIME, getAtime());
        o.put(NFS_CTIME, getCtime());
        o.put(NFS_MTIME, getMtime());
        o.put(NFS_SIZE, getSize());
        o.put(NFS_GENERATION, getGeneration());
        o.put(NFS_LINKS, new JSONArray(metadata.get(NFS_LINKS)));
        o.put(NFS_ACES, new JSONArray(metadata.getOrDefault(NFS_ACES, "[]")));
        return o;
    }

    public InodeMetadata(Stat.Type type, Subject subject, int mode, long fileId) {
        metadata = new HashMap<>();
        metadata.put(NFS_TYPE, type.name());
        int uid = (int) Subjects.getUid(subject);
        long[] gids = Subjects.getGids(subject);
        int gid = gids.length == 0 ? 0 : (int) gids[0];
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
        metadata.put(NFS_LINKS, "[]");
        metadata.put(NFS_ACES, "[]");

    }

    public InodeMetadata(Map<String, String> map) {
        this.metadata = new HashMap<>(map);
    }

    public Map<String, String> asMap() {
        return metadata;
    }

    public InodeMetadata withIncrementedGeneration() {
        Map<String, String> metadata = new HashMap<>(this.metadata);
        long generation = Long.parseLong(metadata.get(NFS_GENERATION));
        metadata.put(NFS_GENERATION, Long.toString(generation + 1l));
        return new InodeMetadata(metadata);
    }

    public int getMode() {
        return Integer.parseInt(metadata.get(NFS_MODE));
    }

    public int getUid() {
        return Integer.parseInt(metadata.get(NFS_UID));
    }

    public int getGid() {
        return Integer.parseInt(metadata.get(NFS_GID));
    }

    public long getGeneration() {
        return Long.parseLong(metadata.get(NFS_GENERATION));
    }

    public long getAtime() {
        return Long.parseLong(metadata.get(NFS_ATIME));
    }

    public long getCtime() {
        return Long.parseLong(metadata.get(NFS_CTIME));
    }

    public long getMtime() {
        return Long.parseLong(metadata.get(NFS_MTIME));
    }

    public long getFileId() {
        try {
            return Long.parseLong(metadata.get(NFS_FILE_ID));
        } catch (NumberFormatException e) {
            throw e;
        }
    }

    public boolean isDirectory() {
        return getType().equals(Stat.Type.DIRECTORY);
    }

    public DirectoryEntry asDirectoryEntry(String entryName, long exportId) {
        return new DirectoryEntry(entryName, asInode(exportId), asStat(exportId));
    }

    public Stat asStat(long exportId) {
        Stat stat = new Stat();
        Stat.Type type = getType();
        int mode = Integer.parseInt(metadata.get(NFS_MODE));
        stat.setMode(type.toMode() | mode);
        stat.setGeneration(Long.parseLong(metadata.get(NFS_GENERATION)));
        stat.setSize(Long.parseLong(metadata.get(NFS_SIZE)));
        stat.setFileid(Long.parseLong(metadata.get(NFS_FILE_ID)));
        stat.setNlink(refCount());
        stat.setUid(Integer.parseInt(metadata.get(NFS_UID)));
        stat.setGid(Integer.parseInt(metadata.get(NFS_GID)));
        stat.setATime(Long.parseLong(metadata.get(NFS_ATIME)));
        stat.setCTime(Long.parseLong(metadata.get(NFS_CTIME)));
        stat.setMTime(Long.parseLong(metadata.get(NFS_MTIME)));
        stat.setDev((int) exportId);
        return stat;
    }

    public int refCount() {
        return new JSONArray(metadata.get(NFS_LINKS)).length();
    }

    public Stat.Type getType() {
        return Enum.valueOf(Stat.Type.class, metadata.get(NFS_TYPE));
    }

    public InodeMetadata withUpdatedMtime() {
        return clone(NFS_MTIME, System.currentTimeMillis());
    }

    public InodeMetadata withUpdatedAtime() {
        return clone(NFS_ATIME, System.currentTimeMillis());
    }

    public InodeMetadata withUpdatedCtime() {
        return clone(NFS_CTIME, System.currentTimeMillis());
    }

    public InodeMetadata withUpdatedTimestamps() {
        return this.withUpdatedCtime()
                .withUpdatedAtime()
                .withUpdatedMtime()
                .withIncrementedGeneration();
    }

    public InodeMetadata withUpdatedSize(long newSize) {
        return clone(NFS_SIZE, newSize);
    }

    private InodeMetadata clone(String fieldName, long newValue) {
        Map<String, String> metadata = new HashMap<>(this.metadata);
        metadata.put(fieldName, Long.toString(newValue));
        return new InodeMetadata(metadata);
    }

    public InodeMetadata withLink(long parentId, String name) {
        Map<String, String> metadata = new HashMap<>(this.metadata);
        JSONArray a = new JSONArray(metadata.get(NFS_LINKS));
        JSONObject o = new JSONObject();
        o.put(PARENT, parentId);
        o.put(NAME, name);
        a.put(o);
        metadata.put(NFS_LINKS, a.toString());
        return new InodeMetadata(metadata);
    }

    public boolean hasLink(long parentId, String name) {
        JSONArray a = new JSONArray(metadata.get(NFS_LINKS));
        for (int i = 0; i < a.length(); i++) {
            JSONObject link = a.getJSONObject(i);
            if (link.getLong(PARENT) == parentId && link.getString(NAME).equals(name)) {
                return true;
            }
        }

        return false;
    }

    public InodeMetadata withoutLink(long parentId, String name) {
        Map<String, String> metadata = new HashMap<>(this.metadata);
        JSONArray a = new JSONArray(metadata.get(NFS_LINKS));
        JSONArray result = new JSONArray();
        for (int i = 0; i < a.length(); i++) {
            JSONObject link = a.getJSONObject(i);
            if (link.getLong(PARENT) == parentId && link.getString(NAME).equals(name)) {

            } else {
                result.put(link);
            }
        }
        metadata.put(NFS_LINKS, result.toString());
        return new InodeMetadata(metadata);
    }

    public long getSize() {
        return Long.parseLong(metadata.get(NFS_SIZE));
    }

    public InodeMetadata update(Stat stat) {
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

        return this;
    }

    public Inode asInode(long exportId) {
        byte[] bytes = new byte[8];
        ByteBuffer buf = ByteBuffer.wrap(bytes);
        buf.putLong(getFileId());
        return new Inode(new FileHandle((int) getGeneration(), (int) exportId, getType().toMode(), bytes));
    }

    public static long fileId(Inode inode) {
        ByteBuffer buf = ByteBuffer.wrap(inode.getFileId());
        return buf.getLong();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        InodeMetadata that = (InodeMetadata) o;

        if (that.metadata.keySet().size() != metadata.keySet().size()) {
            return false;
        }

        for (String theirKey : that.metadata.keySet()) {
            if (!metadata.containsKey(theirKey)) {
                return false;
            }

            if (!metadata.get(theirKey).equals(that.metadata.get(theirKey))) {
                return false;
            }
        }
        return true;
    }

    @Override
    public int hashCode() {
        return metadata.values().stream()
                .mapToInt(o -> o.hashCode())
                .sum();
    }

    public Multimap<Long, String> getLinks() {
        JSONArray a = new JSONArray(metadata.get(NFS_LINKS));
        Multimap<Long, String> map = ArrayListMultimap.create();

        for (int i = 0; i < a.length(); i++) {
            JSONObject link = a.getJSONObject(i);
            map.put(link.getLong(PARENT), link.getString(NAME));
        }

        return map;
    }


    public nfsace4[] getNfsAces() {
        JSONArray array = new JSONArray(metadata.getOrDefault(NFS_ACES, "[]"));
        nfsace4[] result = new nfsace4[array.length()];
        for (int i = 0; i < array.length(); i++) {
            JSONObject o = array.getJSONObject(i);
            nfsace4 ace = new nfsace4();
            ace.access_mask = new acemask4(new uint32_t(o.getInt(ACE_MASK)));
            ace.flag = new aceflag4(new uint32_t(o.getInt(ACE_FLAG)));
            ace.type = new acetype4(new uint32_t(o.getInt(ACE_TYPE)));
            ace.who = new utf8str_mixed(o.getString(ACE_WHO));
            result[i] = ace;
        }
        return result;
    }

    public InodeMetadata withNfsAces(nfsace4... nfsace4s) {
        JSONArray array = new JSONArray();
        for (nfsace4 nfsace4 : nfsace4s) {
            JSONObject o = new JSONObject();
            o.put(ACE_MASK, nfsace4.access_mask.value.value);
            o.put(ACE_FLAG, nfsace4.flag.value.value);
            o.put(ACE_TYPE, nfsace4.type.value.value);
            o.put(ACE_WHO, nfsace4.who.value.toString());
            array.put(o);
        }
        Map<String, String> map = new HashMap<>(metadata);
        map.put(NFS_ACES, array.toString());
        return new InodeMetadata(map);
    }
}

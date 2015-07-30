package com.formationds.nfs;

import com.formationds.protocol.BlobDescriptor;
import org.apache.lucene.document.*;
import org.apache.lucene.index.Term;
import org.apache.lucene.search.PrefixQuery;
import org.apache.lucene.search.Query;
import org.apache.lucene.search.TermQuery;
import org.apache.lucene.util.BytesRefBuilder;
import org.apache.lucene.util.NumericUtils;
import org.dcache.auth.Subjects;
import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;
import org.json.JSONObject;

import javax.security.auth.Subject;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;

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
    public static final String NFS_VOLUME_ID = "NFS_VOLUME_ID";
    public static final String NFS_LINKS = "NFS_LINKS";
    private Map<String, String> metadata;

    public Document asDocument() {
        Document document = new Document();
        document.add(new LongField(NFS_FILE_ID, getFileId(), Field.Store.YES));
        document.add(new StringField(NFS_TYPE, getType().name(), Field.Store.YES));
        document.add(new IntField(NFS_MODE, getMode(), Field.Store.YES));
        document.add(new IntField(NFS_UID, getUid(), Field.Store.YES));
        document.add(new IntField(NFS_GID, getGid(), Field.Store.YES));
        document.add(new LongField(NFS_ATIME, getAtime(), Field.Store.YES));
        document.add(new LongField(NFS_CTIME, getCtime(), Field.Store.YES));
        document.add(new LongField(NFS_MTIME, getMtime(), Field.Store.YES));
        document.add(new LongField(NFS_SIZE, getSize(), Field.Store.YES));
        document.add(new LongField(NFS_GENERATION, getGeneration(), Field.Store.YES));
        document.add(new LongField(NFS_VOLUME_ID, getVolumeId(), Field.Store.YES));
        JSONObject o = new JSONObject(metadata.get(NFS_LINKS));
        Set<String> set = o.keySet();
        for (String parentId : set) {
            String name = o.getString(parentId);
            String fieldValue = parentId + "/" + name;
            document.add(new StringField(NFS_LINKS, fieldValue, Field.Store.YES));
        }
        return document;
    }

    public InodeMetadata(Document document) {
        metadata = new HashMap<>();
        metadata.put(NFS_FILE_ID, Long.toString(document.getField(NFS_FILE_ID).numericValue().longValue()));
        metadata.put(NFS_TYPE, document.get(NFS_TYPE));
        metadata.put(NFS_MODE, Integer.toString(document.getField(NFS_MODE).numericValue().intValue()));
        metadata.put(NFS_UID, Integer.toString(document.getField(NFS_UID).numericValue().intValue()));
        metadata.put(NFS_GID, Integer.toString(document.getField(NFS_GID).numericValue().intValue()));
        metadata.put(NFS_ATIME, Long.toString(document.getField(NFS_ATIME).numericValue().longValue()));
        metadata.put(NFS_CTIME, Long.toString(document.getField(NFS_CTIME).numericValue().longValue()));
        metadata.put(NFS_MTIME, Long.toString(document.getField(NFS_MTIME).numericValue().longValue()));
        metadata.put(NFS_SIZE, Long.toString(document.getField(NFS_SIZE).numericValue().longValue()));
        metadata.put(NFS_GENERATION, Long.toString(document.getField(NFS_GENERATION).numericValue().longValue()));
        metadata.put(NFS_VOLUME_ID, Long.toString(document.getField(NFS_VOLUME_ID).numericValue().longValue()));
        String[] values = document.getValues(NFS_LINKS);
        JSONObject o = new JSONObject();
        for (String value : values) {
            StringTokenizer tokenizer = new StringTokenizer(value, "/", false);
            String parentId = tokenizer.nextToken();
            String name = tokenizer.nextToken();
            o.put(parentId, name);
        }
        metadata.put(NFS_LINKS, o.toString());
    }

    public Term identity() {
        return identity(getFileId());
    }

    private static Term identity(long fileId) {
        BytesRefBuilder bytes = new BytesRefBuilder();
        NumericUtils.longToPrefixCoded(fileId, 0, bytes);
        return new Term(NFS_FILE_ID, bytes.toBytesRef());
    }

    public static Term identity(Inode inode) {
        return identity(fileId(inode));
    }

    public static Query lookupQuery(long parentId, String name) {
        Term t = new Term(NFS_LINKS, Long.toString(parentId) + "/" + name);
        return new TermQuery(t);
    }

    public static Query listQuery(long parentId) {
        return new PrefixQuery(new Term(NFS_LINKS, Long.toString(parentId) + "/"));
    }

    public InodeMetadata(Stat.Type type, Subject subject, int mode, long fileId, long volId) {
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
        metadata.put(NFS_LINKS, new JSONObject().toString());
        metadata.put(NFS_VOLUME_ID, Long.toString(volId));
    }

    private InodeMetadata(Map<String, String> map) {
        this.metadata = map;
    }

    public InodeMetadata(BlobDescriptor blobDescriptor) {
        this.metadata = blobDescriptor.metadata;
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
        return Long.parseLong(metadata.get(NFS_FILE_ID));
    }

    public long getVolumeId() {
        return Long.parseLong(metadata.get(NFS_VOLUME_ID));
    }

    public boolean isDirectory() {
        return getType().equals(Stat.Type.DIRECTORY);
    }

    public DirectoryEntry asDirectoryEntry(long parentDir) {
        String name = new JSONObject(metadata.get(NFS_LINKS)).getString(Long.toString(parentDir));
        return new DirectoryEntry(name, asInode(), asStat());
    }

    public Stat asStat() {
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
        stat.setDev(Integer.parseInt(metadata.get(NFS_VOLUME_ID)));
        return stat;
    }

    public int refCount() {
        return new JSONObject(metadata.get(NFS_LINKS)).length();
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
        JSONObject o = new JSONObject(metadata.get(NFS_LINKS));
        o.put(Long.toString(parentId), name);
        metadata.put(NFS_LINKS, o.toString());
        return new InodeMetadata(metadata);
    }

    public InodeMetadata withoutLink(long parentId) {
        Map<String, String> metadata = new HashMap<>(this.metadata);
        JSONObject o = new JSONObject(metadata.get(NFS_LINKS));
        o.remove(Long.toString(parentId));
        metadata.put(NFS_LINKS, o.toString());
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

        if (stat.isDefined(Stat.StatAttribute.DEV)) {
            metadata.put(NFS_VOLUME_ID, Long.toString(stat.getDev()));
        }

        return this;
    }

    public Inode asInode() {
        byte[] bytes = new byte[8];
        ByteBuffer buf = ByteBuffer.wrap(bytes);
        buf.putLong(getFileId());
        return new Inode(new FileHandle((int) getGeneration(), (int) getVolumeId(), getType().toMode(), bytes));
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
}

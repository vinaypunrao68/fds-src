package com.formationds.nfs;

import com.google.common.collect.Lists;
import org.apache.hadoop.fs.Path;
import org.dcache.nfs.status.ExistException;
import org.dcache.nfs.status.NoEntException;
import org.dcache.nfs.v4.NfsIdMapping;
import org.dcache.nfs.v4.SimpleIdMap;
import org.dcache.nfs.v4.xdr.nfsace4;
import org.dcache.nfs.vfs.*;

import javax.security.auth.Subject;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

class MemoryVirtualFileSystem implements VirtualFileSystem {

    private SimpleIdMap idMap;

    private static class Entry {
        private static final AtomicInteger ID_WELL = new AtomicInteger(10000);
        private final AtomicInteger generation;
        private final long id;
        Stat.Type type;
        String path;

        public Entry(Stat.Type type, String path) {
            this.type = type;
            this.path = path;
            this.id = ID_WELL.incrementAndGet();
            this.generation = new AtomicInteger(0);
        }

        public Inode asInode() {
            return new Inode(new FileHandle(0, 0, 0, path.getBytes()));
        }

        public void incrementGeneration() {
            generation.incrementAndGet();
        }

        public int getMode() {
            int mode = type.toMode();
            if (type.equals(Stat.Type.DIRECTORY)) {
                mode |= 0755;
            } else {
                mode |= 0644;
            }
            return mode;
        }

        public Stat stat() {
            Stat stat = new Stat();
            stat.setMode(getMode());
            stat.setGeneration(generation.get());
            if (type.equals(Stat.Type.DIRECTORY)) {
                stat.setSize(512);
            } else {
                stat.setSize(0);
            }
            stat.setFileid(id);
            stat.setIno((int) id);
            stat.setRdev(13);
            stat.setDev(17);
            stat.setNlink(1);
            stat.setUid(0);
            stat.setGid(0);
            stat.setATime(System.currentTimeMillis());
            stat.setCTime(System.currentTimeMillis());
            stat.setMTime(System.currentTimeMillis());
            return stat;
        }
    }

    private Map<String, Entry> entries;

    public MemoryVirtualFileSystem() {
        entries = new HashMap<>();
        idMap = new SimpleIdMap();
        entries.put("", new Entry(Stat.Type.DIRECTORY, ""));
        entries.put("/exports", new Entry(Stat.Type.DIRECTORY, "/exports"));
    }

    @Override
    public int access(Inode inode, int mode) throws IOException {
        String path = readPath(inode);
        if (!entries.keySet().contains(path)) {
            throw new NoEntException();
        }

        return mode;
    }

    public String readPath(Inode inode) {
        return new String(inode.getFileId());
    }

    @Override
    public Inode create(Inode parent, Stat.Type type, String path, Subject subject, int mode) throws IOException {
        String parentPath = readPath(parent);
        Entry parentEntry = entries.get(parentPath);

        if (parentEntry == null) {
            throw new NoEntException();
        }

        String child = new Path(parentPath, path).toString();
        if (entries.keySet().contains(child)) {
            throw new ExistException();
        }

        Entry entry = new Entry(type, child);
        entries.put(child, entry);
        parentEntry.incrementGeneration();
        return entry.asInode();
    }

    @Override
    public FsStat getFsStat() throws IOException {
        return new FsStat(10 * 1024 * 1024, 0, 0, 0);
    }

    @Override
    public Inode getRootInode() throws IOException {
        return entries.get("").asInode();
    }

    @Override
    public Inode lookup(Inode parent, String path) throws IOException {
        String parentPath = readPath(parent);
        if ("".equals(parentPath)) {
            parentPath = "/";
        }
        String childPath = new Path(parentPath, path).toString();
        if (!entries.keySet().contains(childPath)) {
            throw new NoEntException();
        }

        return entries.get(childPath).asInode();
    }

    @Override
    public Inode link(Inode parent, Inode link, String path, Subject subject) throws IOException {
        throw new RuntimeException("Not implemented");
    }

    @Override
    public List<DirectoryEntry> list(Inode inode) throws IOException {
        String parentPath = readPath(inode);
        if (!entries.keySet().contains(parentPath)) {
            throw new NoEntException();
        }
        return entries.values().stream()
                .filter(e -> !e.path.equals(parentPath))
                .filter(e -> e.path.startsWith(parentPath))
                .map(e -> new DirectoryEntry(new Path(e.path).getName(), e.asInode(), e.stat()))
                .collect(Collectors.toList());
    }

    @Override
    public Inode mkdir(Inode parent, String path, Subject subject, int mode) throws IOException {
        String parentPath = readPath(parent);
        Entry parentEntry = entries.get(parentPath);
        if (parentEntry == null) {
            throw new NoEntException();
        }

        String childPath = new Path(parentPath, path).toString();
        Entry entry = new Entry(Stat.Type.DIRECTORY, childPath);
        entries.put(childPath, entry);
        parentEntry.incrementGeneration();
        return entry.asInode();
    }

    @Override
    public boolean move(Inode src, String oldName, Inode dest, String newName) throws IOException {
        throw new RuntimeException("Not implemented");
    }

    @Override
    public Inode parentOf(Inode inode) throws IOException {
        throw new RuntimeException("Not implemented");
    }

    @Override
    public int read(Inode inode, byte[] data, long offset, int count) throws IOException {
        throw new RuntimeException("Not implemented");
    }

    @Override
    public String readlink(Inode inode) throws IOException {
        throw new RuntimeException("Not implemented");
    }

    @Override
    public void remove(Inode parent, String path) throws IOException {
        String parentPath = readPath(parent);
        if (!entries.keySet().contains(parentPath)) {
            throw new NoEntException();
        }

        String childPath = new Path(parentPath, path).toString();
        if (!entries.keySet().contains(childPath)) {
            throw new NoEntException();
        }

        entries.remove(childPath);
    }

    @Override
    public Inode symlink(Inode parent, String path, String link, Subject subject, int mode) throws IOException {
        throw new RuntimeException("Not implemented");
    }

    @Override
    public WriteResult write(Inode inode, byte[] data, long offset, int count, StabilityLevel stabilityLevel) throws IOException {
        throw new RuntimeException("Not implemented");
    }

    @Override
    public void commit(Inode inode, long offset, int count) throws IOException {
        // no-op
    }

    @Override
    public Stat getattr(Inode inode) throws IOException {
        String path = readPath(inode);
        Entry entry = entries.get(path);
        if (entry == null) {
            throw new NoEntException();
        }
        return entry.stat();
    }

    @Override
    public void setattr(Inode inode, Stat stat) throws IOException {
        System.out.println("SetAttr");
    }

    @Override
    public nfsace4[] getAcl(Inode inode) throws IOException {
        return new nfsace4[0];
    }

    @Override
    public void setAcl(Inode inode, nfsace4[] acl) throws IOException {

    }

    @Override
    public boolean hasIOLayout(Inode inode) throws IOException {
        return false;
    }

    @Override
    public AclCheckable getAclCheckable() {
        return null;
    }

    @Override
    public NfsIdMapping getIdMapper() {
        return idMap;
    }
}

package com.formationds.nfs;

import com.google.common.collect.Lists;
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

class MemoryVirtualFileSystem implements VirtualFileSystem {

    AtomicInteger curId;
    Map<Integer, Node> idMap;

    abstract class Node {
        String name;
        Node parent;
        int id;
        int mode;
        int ctime;
        int atime;
        int mtime;
        int uid;
        int gid;

        public Node(Node parent, String name, int mode, int uid, int gid) {
            this.name = name;
            this.mode = mode;
            id = curId.incrementAndGet();
            idMap.put(id, this);
            int now = (int)(System.currentTimeMillis() / 1000);
            ctime = now;
            atime = now;
            mtime = now;
            this.uid = uid;
            this.gid = gid;
            this.parent = parent;
        }

        boolean isDirectory() {
            return (mode & Stat.S_IFDIR) != 0;
        }

        public abstract FileHandle asFileHandle();
        public abstract Stat stat();
        public abstract Node lookup(List<String> path) throws IOException;

        public Inode asInode() {
            return new Inode(asFileHandle());
        }

        public DirectoryEntry asDirectoryEntry(String localName) {
            return new DirectoryEntry(localName, asInode(), stat());
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            if (parent != null) {
                sb.append(parent.toString());
                sb.append("/");
                sb.append(name);
            }

            return sb.toString();
        }
    }

    class Directory extends Node {
        Map<String, Node> entries;

        public Directory(Node parent, String name, int uid, int gid) {
            super(parent, name, Stat.S_IFDIR, uid, gid);
            entries = new HashMap<>();
        }

        @Override
        public Node lookup(List<String> path) throws IOException{
            if(path.size() == 0)
                return this;

            String name = path.get(0);
            if(entries.containsKey(name)) {
                return entries.get(name).lookup(path.subList(1, path.size()));
            }

            throw new NoEntException();
        }


        public Stat stat() {
            Stat stat = new Stat();
            stat.setMode(Stat.S_IFDIR | 0755);
            stat.setGeneration(0);
            stat.setSize(0);
            stat.setFileid(id);
            stat.setNlink(1); // FIXME: implement linking
            stat.setUid(uid);
            stat.setGid(gid);
            stat.setATime(ctime);
            stat.setCTime(atime);
            stat.setMTime(mtime);
            return stat;
        }

        @Override
        public FileHandle asFileHandle() {
            return new FileHandle(0, 0, Stat.S_IFDIR, ByteBuffer.allocate(4).putInt(id).array());
        }

        public synchronized void add(String name, Node node) throws IOException {
            if(entries.containsKey(name))
                throw new IOException("A file or directory with that name already exists");

            entries.put(name, node);
        }

        public synchronized void remove(String name) {
            // FIXME: adjust number of links when linking is implemented
            entries.remove(name);
        }
    }

    class File extends Node {
        ByteBuffer content;

        public File(Node parent, String name, int uid, int gid, ByteBuffer data) {
            super(parent, name, Stat.S_IFREG, uid, gid);
            this.content = data;
        }

        @Override
        public FileHandle asFileHandle() {
            return new FileHandle(0, 0, Stat.S_IFREG, ByteBuffer.allocate(4).putInt(id).array());
        }

        @Override
        public Node lookup(List<String> path) throws IOException {
            if(path.size() == 0)
                return this;
            throw new FileNotFoundException();
        }

        @Override
        public Stat stat() {
            Stat stat = new Stat();
            stat.setMode(Stat.S_IFREG | 0644);
            stat.setGeneration(0);
            stat.setSize(content.remaining());
            stat.setFileid(id);
            stat.setNlink(1); // FIXME: implement linking
            stat.setUid(uid);
            stat.setGid(gid);
            stat.setATime(ctime);
            stat.setCTime(atime);
            stat.setMTime(mtime);
            return stat;
        }

        public int write(byte[] data, int offset, int length) {
            if(offset + length > content.remaining()) {
                ByteBuffer buf = ByteBuffer.allocate(offset + length);
                buf.put(content);
                buf.put(data, offset, length);
                buf.position(0);
                content = buf;
            } else {
                ByteBuffer view = content.duplicate();
                view.put(data, offset, length);
            }

            return length;
        }

        public int read(byte[] buf, int offset, int length) {
            ByteBuffer view = content.duplicate();
            view.position(offset);
            int readAmount = Math.min(view.remaining(), length);
            view.get(buf, 0, readAmount);
            return readAmount == 0 ? -1 : readAmount;
        }
    }

    private Directory root;

    public MemoryVirtualFileSystem() {
        curId = new AtomicInteger(0);
        idMap = new HashMap<>();
        root = new Directory(null, "", 0, 0);
    }

    private Node nodeFromInode(Inode inode) throws IOException {
        int id = getId(inode);
        if(!idMap.containsKey(id))
            throw new NoEntException();

        return idMap.get(id);
    }

    private int getId(Inode inode) {
        return ByteBuffer.wrap(inode.getFileId()).getInt();
    }

    private File fileFromInode(Inode inode) throws IOException {
        Node n = nodeFromInode(inode);
        if(n.isDirectory())
            throw new IOException("Expecting node to be a file, but it is not");
        return (File)n;
    }

    private Directory directoryFromInode(Inode inode) throws IOException {
        Node n = nodeFromInode(inode);
        if(!n.isDirectory())
            throw new IOException("Expecting node to be a file, but it is not");
        return (Directory)n;
    }

    @Override
    public int access(Inode inode, int mode) throws IOException {
        Node node = nodeFromInode(inode);
        idMap.containsKey(getId(inode));
        System.out.println("Node: " + node);
        return mode;
    }

    @Override
    public Inode create(Inode parent, Stat.Type type, String path, Subject subject, int mode) throws IOException {
        // FIXME: we are ignoring subject
        // we should do something like ChimeraVFS: eg
        // int uid = (int)Subjects.getUid(subject);
        // int gid = (int)Subjects.getPrimaryGid(subject);

        Directory parentDir = directoryFromInode(parent);
        if(type == Stat.Type.DIRECTORY) {
            Directory directory = new Directory(parentDir, path, 0, 0);
            parentDir.add(path, directory);
            return directory.asInode();
        } else if(type == Stat.Type.REGULAR) {
            File file = new File(parentDir, path, 0, 0, ByteBuffer.allocate(0));
            parentDir.add(path, file);
            return file.asInode();
        }

        throw new IOException("Don't know how to make type " + type.toString());
    }

    @Override
    public FsStat getFsStat() throws IOException {
        // FIXME: implement
        return new FsStat(1024 * 1024 * 1024, 1024, 1024, 1024);
    }

    @Override
    public Inode getRootInode() throws IOException {
         return new Inode(root.asFileHandle());
    }

    @Override
    public Inode lookup(Inode parent, String path) throws IOException {
        List<String> pathList = Lists.newArrayList(path.split("/"));
        Directory directory = directoryFromInode(parent);
        Node lookup = directory.lookup(pathList);
        if (lookup == null) {
            throw new NoEntException("File not found");
        }
        return lookup.asInode();
    }

    @Override
    public List<DirectoryEntry> list(Inode inode) throws IOException {
        ArrayList<DirectoryEntry> entries = new ArrayList<>();
        Directory dir = directoryFromInode(inode);
        for(Map.Entry<String, Node> entry : dir.entries.entrySet()) {
            entries.add(entry.getValue().asDirectoryEntry(entry.getKey()));
        }
        return entries;
    }

    @Override
    public Inode mkdir(Inode parent, String path, Subject subject, int mode) throws IOException {
        return create(parent, Stat.Type.DIRECTORY, path, subject, mode);
    }

    @Override
    public boolean move(Inode src, String oldName, Inode dest, String newName) throws IOException {
        Directory sourceDir = directoryFromInode(src);
        Directory destDir = directoryFromInode(dest);
        Node node = sourceDir.lookup(Collections.singletonList(oldName));
        destDir.add(newName, node);
        sourceDir.remove(oldName);
        return true;
    }

    @Override
    public void remove(Inode parent, String path) throws IOException {
        Inode node = lookup(parent, path);
        directoryFromInode(parent).remove(path);
        idMap.remove(getId(node));
    }

    @Override
    public WriteResult write(Inode inode, byte[] data, long offset, int count, StabilityLevel stabilityLevel) throws IOException {
        int writeLength = fileFromInode(inode).write(data, (int)offset, count);
        return new WriteResult(stabilityLevel, writeLength);
    }

    @Override
    public Inode parentOf(Inode inode) throws IOException {
        return nodeFromInode(inode).parent.asInode();
    }

    @Override
    public int read(Inode inode, byte[] data, long offset, int count) throws IOException {
        return fileFromInode(inode).read(data, (int)offset, count);
    }

    @Override
    public String readlink(Inode inode) throws IOException {
        return null;
    }


    @Override
    public Inode symlink(Inode parent, String path, String link, Subject subject, int mode) throws IOException {
        return null;
    }

    @Override
    public Inode link(Inode parent, Inode link, String path, Subject subject) throws IOException {
        return null;
    }

    @Override
    public void commit(Inode inode, long offset, int count) throws IOException {
        System.out.println("Commit");
    }

    @Override
    public Stat getattr(Inode inode) throws IOException {

        return nodeFromInode(inode).stat();
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
        System.out.println("SetAcl");
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
        return new SimpleIdMap();
    }
}

package com.formationds.nfs;

import org.apache.log4j.Logger;
import org.dcache.nfs.status.NoEntException;
import org.dcache.nfs.status.NoSpcException;
import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;

import javax.security.auth.Subject;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Map;
import java.util.Optional;

public class InodeMap {
    private static final Logger LOG = Logger.getLogger(InodeMap.class);
    private final IoOps io;
    private PersistentCounter usedBytes;
    private PersistentCounter usedFiles;
    private final ExportResolver exportResolver;
    public static final Inode ROOT;
    public static final InodeMetadata ROOT_METADATA;

    static {
        byte[] bytes = new byte[8];
        ByteBuffer bb = ByteBuffer.wrap(bytes);
        bb.putLong(Long.MAX_VALUE);
        ROOT = new Inode(new FileHandle(0, Integer.MAX_VALUE, Stat.Type.DIRECTORY.toMode(), bytes));
        ROOT_METADATA = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0755, Long.MAX_VALUE);
    }

    private final Chunker chunker;

    public InodeMap(IoOps io, ExportResolver exportResolver) {
        this.exportResolver = exportResolver;
        this.io = io;
        this.usedBytes = new PersistentCounter(io, XdiVfs.DOMAIN, "usedBytes", 0);
        this.usedFiles = new PersistentCounter(io, XdiVfs.DOMAIN, "usedFiles", 0);
        chunker = new Chunker(io);
    }

    public Optional<InodeMetadata> stat(Inode inode) throws IOException {
        if (isRoot(inode)) {
            return Optional.of(ROOT_METADATA);
        }

        String blobName = blobName(inode);
        String volumeName = volumeName(inode);

        Optional<FdsMetadata> opt = io.readMetadata(XdiVfs.DOMAIN, volumeName, blobName);
        if (opt.isPresent()) {
            InodeMetadata inodeMetadata = opt.get().lock(m -> {
                if (m.mutableMap().size() == 0) {
                    LOG.error("Metadata for inode-" + InodeMetadata.fileId(inode) + " is empty!");
                    throw new NoEntException();
                }
                return new InodeMetadata(m.mutableMap());
            });
            return Optional.of(inodeMetadata);
        } else {
            return Optional.empty();
        }
    }

    public static String blobName(Inode inode) {
        return "inode-" + InodeMetadata.fileId(inode);
    }

    public static String blobName(long fileId) {
        return "inode-" + fileId;
    }

    public long usedBytes(String volume) throws IOException {
        try {
            return usedBytes.currentValue(volume);
        } catch (IOException e) {
            LOG.error("Error polling " + volume + ".usedBytes", e);
            return 0;
        }
    }

    public long usedFiles(String volume) throws IOException {
        try {
            return usedFiles.currentValue(volume);
        } catch (IOException e) {
            LOG.error("Error polling " + volume + ".usedFiles", e);
            return 0;
        }
    }

    public InodeMetadata write(Inode inode, byte[] data, long offset, int count) throws IOException {
        String volumeName = exportResolver.volumeName(inode.exportIndex());
        String blobName = InodeMap.blobName(inode);

        try {
            int objectSize = exportResolver.objectSize(volumeName);
            InodeMetadata[] last = new InodeMetadata[1];
            chunker.write(XdiVfs.DOMAIN, volumeName, blobName, objectSize, data, offset, count, map -> {
                if (map.isEmpty()) {
                    throw new NoEntException();
                }
                InodeMetadata inodeMetadata = new InodeMetadata(map);
                long oldByteCount = inodeMetadata.getSize();
                int length = Math.min(data.length, count);
                long newByteCount = Math.max(oldByteCount, offset + length);
                enforceVolumeLimit(volumeName, newByteCount - oldByteCount);
                last[0] = inodeMetadata
                        .withUpdatedAtime()
                        .withUpdatedMtime()
                        .withUpdatedCtime()
                        .withUpdatedSize(newByteCount);
                usedBytes.increment(volumeName, newByteCount - oldByteCount);
                map.clear();
                map.putAll(last[0].asMap());
                return null;
            });
            return last[0];
        } catch (IOException e) {
            String message = "Error writing " + volumeName + "." + blobName;
            LOG.error(message, e);
            throw e;
        }
    }

    private void enforceVolumeLimit(String volumeName, long increment) throws IOException {
        long currentlyUsedBytes = usedBytes(volumeName);
        if (currentlyUsedBytes + increment > exportResolver.maxVolumeCapacityInBytes(volumeName)) {
            throw new NoSpcException();
        }
    }

    public Inode create(InodeMetadata metadata, long exportId) throws IOException {
        String volumeName = exportResolver.volumeName((int) exportId);
        Inode inode = doUpdate(metadata, exportId);
        usedFiles.increment(volumeName);
        return inode;
    }

    private Inode doUpdate(InodeMetadata metadata, long exportId) throws IOException {
        String volume = exportResolver.volumeName((int) exportId);
        String blobName = blobName(metadata.asInode(exportId));
        return io.readMetadata(XdiVfs.DOMAIN, volume, blobName).orElse(new FdsMetadata()).lock(m -> {
            Map<String, String> mutableMap = m.mutableMap();
            mutableMap.putAll(metadata.asMap());
            io.writeMetadata(XdiVfs.DOMAIN, volume, blobName, m.fdsMetadata());
            return metadata.asInode(exportId);
        });
    }

    public int volumeId(Inode inode) {
        return inode.exportIndex();
    }

    public String volumeName(Inode inode) {
        return exportResolver.volumeName(inode.exportIndex());
    }

    public long fileId(Inode inode) {
        return InodeMetadata.fileId(inode);
    }

    public void update(long exportId, InodeMetadata... entries) throws IOException {
        for (InodeMetadata entry : entries) {
            doUpdate(entry, exportId);
        }
    }

    public void remove(Inode inode) throws IOException {
        String blobName = blobName(inode);
        String volumeName = volumeName(inode);

        try {
            Optional<InodeMetadata> om = stat(inode);
            io.deleteBlob(XdiVfs.DOMAIN, volumeName, blobName);
            if (om.isPresent()) {
                usedBytes.decrement(volumeName, om.get().getSize());
                usedFiles.decrement(volumeName, 1);
            }
        } catch (Exception e) {
            throw new IOException(e);
        }
    }

    public static boolean isRoot(Inode inode) {
        return InodeMetadata.fileId(inode) == Long.MAX_VALUE;
    }
}

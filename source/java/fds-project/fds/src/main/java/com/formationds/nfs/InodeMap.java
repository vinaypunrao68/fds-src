package com.formationds.nfs;

import org.apache.log4j.Logger;
import org.dcache.nfs.status.NoEntException;
import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;

import javax.security.auth.Subject;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Optional;

public class InodeMap {
    private static final Logger LOG = Logger.getLogger(InodeMap.class);

    private final TransactionalIo io;
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

    public InodeMap(TransactionalIo io, ExportResolver exportResolver) {
        this.exportResolver = exportResolver;
        this.io = io;
        this.usedBytes = new PersistentCounter(io, "usedBytes", 0, true);
        this.usedFiles = new PersistentCounter(io, "usedFiles", 0, true);
        chunker = new Chunker(io);
    }

    public Optional<InodeMetadata> stat(Inode inode) throws IOException {
        if (isRoot(inode)) {
            return Optional.of(ROOT_METADATA);
        }

        String blobName = blobName(inode);
        String volumeName = volumeName(inode);

        return io.mapMetadata(XdiVfs.DOMAIN, volumeName, blobName, (name, om) -> {
            if (om.isPresent() && om.get().size() == 0) {
                LOG.error("Metadata for inode-" + InodeMetadata.fileId(inode) + " is empty!");
                throw new NoEntException();
            }
            return om.map(m -> new InodeMetadata(m));
        });
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
        } catch (Exception e) {
            String message = "Error writing " + volumeName + "." + blobName;
            throw new IOException(message, e);
        }
    }

    public Inode create(InodeMetadata metadata, long exportId, boolean deferrable) throws IOException {
        String volumeName = exportResolver.volumeName((int) exportId);
        Inode inode = doUpdate(metadata, exportId, deferrable);
        usedFiles.increment(volumeName);
        return inode;
    }

    private Inode doUpdate(InodeMetadata metadata, long exportId, boolean deferrable) throws IOException {
        String volume = exportResolver.volumeName((int) exportId);
        String blobName = blobName(metadata.asInode(exportId));
        io.mutateMetadata(XdiVfs.DOMAIN, volume, blobName, deferrable, (x) -> {
            x.clear();
            x.putAll(metadata.asMap());
            return null;
        });
        return metadata.asInode(exportId);
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

    public void update(long exportId, boolean deferrable, InodeMetadata... entries) throws IOException {
        for (InodeMetadata entry : entries) {
            doUpdate(entry, exportId, deferrable);
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

package com.formationds.nfs;

import org.dcache.nfs.status.NoEntException;
import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;

import javax.security.auth.Subject;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Map;
import java.util.Optional;

public class InodeMap {
    private final Io io;
    private final ExportResolver exportResolver;
    public static final Inode ROOT;
    public static final InodeMetadata ROOT_METADATA;

    static {
        byte[] bytes = new byte[8];
        ByteBuffer bb = ByteBuffer.wrap(bytes);
        bb.putLong(Long.MAX_VALUE);
        ROOT = new Inode(new FileHandle(0, Integer.MAX_VALUE, Stat.Type.DIRECTORY.toMode(), bytes));
        ROOT_METADATA = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0755, Long.MAX_VALUE, Integer.MAX_VALUE);
    }

    private final Chunker chunker;

    public InodeMap(Io io, ExportResolver exportResolver) {
        this.exportResolver = exportResolver;
        this.io = io;
        chunker = new Chunker(io);
    }


    public Optional<InodeMetadata> stat(Inode inode) throws IOException {
        if (isRoot(inode)) {
            return Optional.of(ROOT_METADATA);
        }

        String blobName = blobName(inode);
        String volumeName = volumeName(inode);

        return io.mapMetadata(BlockyVfs.DOMAIN, volumeName, blobName, (x) -> x)
                .map(m -> new InodeMetadata(m));
    }

    public static String blobName(Inode inode) {
        return "inode-" + InodeMetadata.fileId(inode);
    }

    public InodeMetadata write(Inode inode, byte[] data, long offset, int count) throws IOException {
        String volumeName = exportResolver.volumeName(inode.exportIndex());
        String blobName = InodeMap.blobName(inode);

        try {
            int objectSize = exportResolver.objectSize(volumeName);
            InodeMetadata[] last = new InodeMetadata[1];
            chunker.write(BlockyVfs.DOMAIN, volumeName, blobName, objectSize, data, offset, count, new MetadataMutator() {
                @Override
                public Map<String, String> mutateOrCreate(Optional<Map<String, String>> om) throws IOException {
                    if (!om.isPresent()) {
                        throw new NoEntException();
                    }
                    InodeMetadata inodeMetadata = new InodeMetadata(om.get());
                    long byteCount = inodeMetadata.getSize();
                    int length = Math.min(data.length, count);
                    byteCount = Math.max(byteCount, offset + length);
                    last[0] = inodeMetadata
                            .withUpdatedAtime()
                            .withUpdatedMtime()
                            .withUpdatedCtime()
                            .withUpdatedSize(byteCount);
                    return last[0].asMap();
                }
            });
            return last[0];
        } catch (Exception e) {
            String message = "Error writing " + volumeName + "." + blobName;
            throw new IOException(message, e);
        }
    }

    public Inode create(InodeMetadata metadata) throws IOException {
        return doUpdate(metadata);
    }

    private Inode doUpdate(InodeMetadata metadata) throws IOException {
        String volume = exportResolver.volumeName((int) metadata.getVolumeId());
        String blobName = blobName(metadata.asInode());
        io.mutateMetadata(BlockyVfs.DOMAIN, volume, blobName, (x) -> metadata.asMap());
        return metadata.asInode();
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

    public void update(InodeMetadata... entries) throws IOException {
        for (InodeMetadata entry : entries) {
            doUpdate(entry);
        }
    }

    public void remove(Inode inode) throws IOException {
        String blobName = blobName(inode);
        String volumeName = volumeName(inode);

        try {
            io.deleteBlob(BlockyVfs.DOMAIN, volumeName, blobName);
        } catch (Exception e) {
            throw new IOException(e);
        } finally {
            //cache.invalidate(key);
        }
    }

    public static boolean isRoot(Inode inode) {
        return InodeMetadata.fileId(inode) == Long.MAX_VALUE;
    }
}

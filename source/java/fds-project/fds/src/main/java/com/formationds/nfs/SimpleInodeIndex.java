package com.formationds.nfs;

import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.Inode;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

public class SimpleInodeIndex implements InodeIndex {
    private Io io;
    private ExportResolver exportResolver;

    public SimpleInodeIndex(Io io, ExportResolver exportResolver) {
        this.io = io;
        this.exportResolver = exportResolver;
    }

    @Override
    public Optional<InodeMetadata> lookup(Inode parent, String name) throws IOException {
        String volumeName = exportResolver.volumeName(parent.exportIndex());
        String blobName = blobName(InodeMetadata.fileId(parent), name);
        return io.mapMetadata(BlockyVfs.DOMAIN, volumeName, blobName,
                metadata -> metadata.map(m -> new InodeMetadata(m)));
    }

    @Override
    public List<DirectoryEntry> list(InodeMetadata directory, long exportId) throws IOException {
        String volumeName = exportResolver.volumeName((int) exportId);
        return io.scan(
                BlockyVfs.DOMAIN,
                volumeName,
                "index-" + directory.getFileId() + "/",
                md -> new InodeMetadata(md.get())
                        .asDirectoryEntry(directory.getFileId(), exportId));
    }

    @Override
    public void index(long exportId, InodeMetadata... entries) throws IOException {
        for (InodeMetadata entry : entries) {
            String volumeName = exportResolver.volumeName((int) exportId);
            Map<Long, String> links = entry.getLinks();
            for (long parentId : links.keySet()) {
                String blobName = blobName(parentId, links.get(parentId));
                io.mutateMetadata(BlockyVfs.DOMAIN, volumeName, blobName, metadata -> {
                    metadata.clear();
                    metadata.putAll(entry.asMap());
                });
            }
        }
    }

    private String blobName(long parentId, String linkName) {
        return "index-" + parentId + "/" + linkName;
    }

    @Override
    public void remove(long exportId, InodeMetadata inodeMetadata) throws IOException {
        Set<Long> parents = inodeMetadata.getLinks().keySet();
        for (Long parentId : parents) {
            unlink(exportId, parentId, inodeMetadata.getLinks().get(parentId));
        }
    }

    @Override
    public void unlink(long exportId, long parentId, String linkName) throws IOException {
        String volumeName = exportResolver.volumeName((int) exportId);
        String blobName = blobName(parentId, linkName);
        io.deleteBlob(BlockyVfs.DOMAIN, volumeName, blobName);
    }
}

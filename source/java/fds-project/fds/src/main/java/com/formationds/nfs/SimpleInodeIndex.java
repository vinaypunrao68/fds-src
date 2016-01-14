package com.formationds.nfs;

import com.google.common.collect.Multimap;
import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.Inode;

import java.io.IOException;
import java.util.Collection;
import java.util.List;
import java.util.Optional;
import java.util.Set;

public class SimpleInodeIndex implements InodeIndex {
    private TransactionalIo io;
    private ExportResolver exportResolver;

    public SimpleInodeIndex(TransactionalIo io, ExportResolver exportResolver) {
        this.io = io;
        this.exportResolver = exportResolver;
    }

    @Override
    public Optional<InodeMetadata> lookup(Inode parent, String name) throws IOException {
        String volumeName = exportResolver.volumeName(parent.exportIndex());
        String blobName = blobName(InodeMetadata.fileId(parent), name);
        return io.mapMetadata(XdiVfs.DOMAIN, volumeName, blobName,
                (x, metadata) -> metadata.map(m -> new InodeMetadata(m)));
    }

    @Override
    public List<DirectoryEntry> list(InodeMetadata directory, long exportId) throws IOException {
        String volumeName = exportResolver.volumeName((int) exportId);
        String blobNamePrefix = "index-" + directory.getFileId() + "/";
        return io.scan(
                XdiVfs.DOMAIN,
                volumeName,
                blobNamePrefix,
                (blobName, md) -> new InodeMetadata(md.get())
                        .asDirectoryEntry(blobName.replace(blobNamePrefix, ""), exportId));
    }

    @Override
    public void index(long exportId, boolean deferrable, InodeMetadata entry) throws IOException {
        String volumeName = exportResolver.volumeName((int) exportId);
        Multimap<Long, String> links = entry.getLinks();
        for (long parentId : links.keySet()) {
            Collection<String> names = links.get(parentId);
            for (String name : names) {
                String blobName = blobName(parentId, name);
                io.mutateMetadata(XdiVfs.DOMAIN, volumeName, blobName, entry.asMap(), deferrable);
            }
        }
    }

    private String blobName(long parentId, String linkName) {
        return "index-" + parentId + "/" + linkName;
    }

    @Override
    public void remove(long exportId, InodeMetadata inodeMetadata) throws IOException {
        Multimap<Long, String> links = inodeMetadata.getLinks();
        Set<Long> parents = links.keySet();
        for (Long parentId : parents) {
            Collection<String> names = links.get(parentId);
            for (String name : names) {
                unlink(exportId, parentId, name);
            }
        }
    }

    @Override
    public void unlink(long exportId, long parentId, String linkName) throws IOException {
        String volumeName = exportResolver.volumeName((int) exportId);
        String blobName = blobName(parentId, linkName);
        io.deleteBlob(XdiVfs.DOMAIN, volumeName, blobName);
    }
}

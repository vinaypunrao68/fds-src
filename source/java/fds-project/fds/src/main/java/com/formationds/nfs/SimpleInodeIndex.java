package com.formationds.nfs;

import com.google.common.collect.Multimap;
import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.Inode;

import java.io.IOException;
import java.util.*;

public class SimpleInodeIndex implements InodeIndex {
    private IoOps io;
    private ExportResolver exportResolver;

    public SimpleInodeIndex(IoOps io, ExportResolver exportResolver) {
        this.io = io;
        this.exportResolver = exportResolver;
    }

    @Override
    public Optional<InodeMetadata> lookup(Inode parent, String name) throws IOException {
        String volumeName = exportResolver.volumeName(parent.exportIndex());
        String blobName = blobName(InodeMetadata.fileId(parent), name);
        Optional<FdsMetadata> fdsMetadata = io.readMetadata(XdiVfs.DOMAIN, volumeName, blobName);
        if (!fdsMetadata.isPresent()) {
            return Optional.empty();
        } else {
            return Optional.of(fdsMetadata.get().lock(m -> new InodeMetadata(m.mutableMap())));
        }
    }

    @Override
    public List<DirectoryEntry> list(InodeMetadata directory, long exportId) throws IOException {
        String volumeName = exportResolver.volumeName((int) exportId);
        String blobNamePrefix = "index-" + directory.getFileId() + "/";
        Collection<BlobMetadata> scanResult = io.scan(XdiVfs.DOMAIN, volumeName, blobNamePrefix);
        List<DirectoryEntry> result = new ArrayList<>(scanResult.size());
        for (BlobMetadata blobMetadata : scanResult) {
            InodeMetadata im = blobMetadata.getMetadata().lock(m -> new InodeMetadata(m.mutableMap()));
            String blobName = blobMetadata.getBlobName().replace(blobNamePrefix, "");
            result.add(im.asDirectoryEntry(blobName, exportId));
        }
        return result;
    }

    @Override
    public void index(long exportId, boolean deferrable, InodeMetadata entry) throws IOException {
        String volumeName = exportResolver.volumeName((int) exportId);
        Multimap<Long, String> links = entry.getLinks();
        for (long parentId : links.keySet()) {
            Collection<String> names = links.get(parentId);
            for (String name : names) {
                String blobName = blobName(parentId, name);
                io.writeMetadata(XdiVfs.DOMAIN, volumeName, blobName, new FdsMetadata(entry.asMap()));
                if (!deferrable) {
                    io.commitMetadata(XdiVfs.DOMAIN, volumeName, blobName);
                }
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

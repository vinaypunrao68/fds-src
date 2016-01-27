package com.formationds.nfs;

import com.formationds.xdi.AsyncAm;
import com.google.common.base.Joiner;
import com.google.common.collect.Sets;
import org.apache.log4j.Logger;
import org.dcache.acl.ACE;
import org.dcache.auth.GidPrincipal;
import org.dcache.auth.UidPrincipal;
import org.dcache.nfs.FsExport;
import org.dcache.nfs.status.ExistException;
import org.dcache.nfs.status.NoEntException;
import org.dcache.nfs.status.NotDirException;
import org.dcache.nfs.v4.NfsIdMapping;
import org.dcache.nfs.v4.SimpleIdMap;
import org.dcache.nfs.v4.xdr.nfsace4;
import org.dcache.nfs.vfs.*;
import org.joda.time.Duration;

import javax.security.auth.Subject;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Optional;

public class XdiVfs implements VirtualFileSystem, AclCheckable {
    private static final Logger LOG = Logger.getLogger(XdiVfs.class);
    private InodeMap inodeMap;
    private PersistentCounter allocator;
    private InodeIndex inodeIndex;
    private ExportResolver exportResolver;
    private SimpleIdMap idMap;
    private Chunker chunker;
    private final Counters counters;

    public static final String DOMAIN = "nfs";
    public static final long MIN_FILE_ID = 256;
    public static final String FILE_ID_WELL = "file-id-well";

    public XdiVfs(AsyncAm asyncAm, ExportResolver resolver, Counters counters, boolean deferMetadataWrites, int amRetryAttempts, Duration amRetryInterval) {
        IoOps ops = new RecoveryHandler(new AmOps(asyncAm, counters), amRetryAttempts, amRetryInterval);
        if (deferMetadataWrites) {
            DeferredIoOps deferredOps = new DeferredIoOps(ops, counters);
            ops = deferredOps;
            resolver.addVolumeDeleteEventHandler(v -> deferredOps.onVolumeDeletion(DOMAIN, v));
            ((DeferredIoOps) ops).start();
        }
        inodeMap = new InodeMap(ops, resolver);
        allocator = new PersistentCounter(ops, DOMAIN, FILE_ID_WELL, MIN_FILE_ID, false);
        this.exportResolver = resolver;
        inodeIndex = new SimpleInodeIndex(ops, resolver);
        idMap = new SimpleIdMap();
        chunker = new Chunker(ops);
        this.counters = counters;
    }

    @Override
    public int access(Inode inode, int mode) throws IOException {
        counters.increment(Counters.Key.inodeAccess);
        Optional<InodeMetadata> stat = inodeMap.stat(inode);
        if (!stat.isPresent()) {
            throw new NoEntException();
        }
        return mode;
    }

    @Override
    public Inode create(Inode parent, Stat.Type type, String name, Subject subject, int mode) throws IOException {
        counters.increment(Counters.Key.inodeCreate);
        Optional<InodeMetadata> parentMetadata = inodeMap.stat(parent);
        if (!parentMetadata.isPresent()) {
            throw new NoEntException();
        }

        Optional<InodeMetadata> childMetadata = inodeIndex.lookup(parent, name);
        if (childMetadata.isPresent()) {
            throw new ExistException();
        }

        if (!parentMetadata.get().isDirectory()) {
            throw new NotDirException();
        }

        String volume = exportResolver.volumeName(parent.exportIndex());
        long fileId = allocator.increment(volume);
        LOG.debug("Allocating fileID [" + fileId + "] for file [" + name + "]");
        InodeMetadata metadata = new InodeMetadata(type, subject, mode, fileId)
                .withLink(inodeMap.fileId(parent), name);

        InodeMetadata updatedParent = parentMetadata.get().withUpdatedTimestamps();
        Inode createdInode = inodeMap.create(metadata, parent.exportIndex(), true);
        inodeMap.update(parent.exportIndex(), true, updatedParent);
        inodeIndex.index(parent.exportIndex(), true, metadata);
        inodeIndex.index(parent.exportIndex(), true, updatedParent);
        LOG.debug("Created " + InodeMap.blobName(metadata.getFileId()));
        return createdInode;
    }

    @Override
    public FsStat getFsStat() throws IOException {
        long totalFiles = 0;
        long usedBytes = 0;
        Collection<String> volumes = exportResolver.exportNames();
        for (String volume : volumes) {
            usedBytes += inodeMap.usedBytes(volume);
            totalFiles += inodeMap.usedFiles(volume);
        }
        return new FsStat(1024l * 1024l * 1024l * 1024l * 1024l, Long.MAX_VALUE, usedBytes, totalFiles);
    }

    public FsStat getFsStat(int exportIndex) throws IOException {
        String volumeName = exportResolver.volumeName(exportIndex);
        long usedSpace = inodeMap.usedBytes(volumeName);
        long usedFiles = inodeMap.usedFiles(volumeName);
        return new FsStat(1024l * 1024l * 1024l * 1024l * 1024l, Long.MAX_VALUE, usedSpace, usedFiles);
    }

    @Override
    public Inode getRootInode() throws IOException {
        return InodeMap.ROOT;
    }

    @Override
    public Inode lookup(Inode parent, String path) throws IOException {
        counters.increment(Counters.Key.lookupFile);
        if (InodeMap.isRoot(parent)) {
            String volumeName = path;
            if (!exportResolver.exists(volumeName)) {
                throw new NoEntException();
            }

            int exportId = (int) exportResolver.exportId(volumeName);
            Subject nobodyUser = new Subject(
                    true,
                    Sets.newHashSet(
                            new UidPrincipal(FsExport.DEFAULT_ANON_UID),
                            new GidPrincipal(FsExport.DEFAULT_ANON_GID,
                                    true)),
                    Sets.newHashSet(),
                    Sets.newHashSet());

            InodeMetadata inodeMetadata = new InodeMetadata(Stat.Type.DIRECTORY, nobodyUser, 0755, MIN_FILE_ID)
                    .withUpdatedAtime()
                    .withUpdatedCtime()
                    .withUpdatedMtime()
                    .withLink(MIN_FILE_ID, ".");

            Optional<InodeMetadata> statResult = inodeMap.stat(inodeMetadata.asInode(exportId));
            if (!statResult.isPresent()) {
                LOG.debug("Creating export root for volume " + path);
                Inode inode = inodeMap.create(inodeMetadata, exportId, false);
                inodeIndex.index(exportId, false, inodeMetadata);
                return inode;
            } else {
                return statResult.get().asInode(exportId);
            }
        }

        Optional<InodeMetadata> oim = inodeIndex.lookup(parent, path);
        if (!oim.isPresent()) {
            throw new NoEntException();
        }

        return oim.get().asInode(parent.exportIndex());
    }

    @Override
    public Inode link(Inode parent, Inode link, String path, Subject subject) throws IOException {
        counters.increment(Counters.Key.link);
        Optional<InodeMetadata> parentMetadata = inodeMap.stat(parent);
        if (!parentMetadata.isPresent()) {
            throw new NoEntException();
        }
        if (!parentMetadata.get().isDirectory()) {
            throw new NotDirException();
        }

        Optional<InodeMetadata> linkMetadata = inodeMap.stat(link);
        if (!linkMetadata.isPresent()) {
            throw new NoEntException();
        }

        InodeMetadata updatedParentMetadata = parentMetadata.get().withUpdatedTimestamps();
        InodeMetadata updatedLinkMetadata = linkMetadata.get().withLink(inodeMap.fileId(parent), path);
        inodeMap.update(parent.exportIndex(), true, updatedParentMetadata, updatedLinkMetadata);
        inodeIndex.index(parent.exportIndex(), true, updatedParentMetadata);
        inodeIndex.index(parent.exportIndex(), true, updatedLinkMetadata);
        return link;
    }

    @Override
    public List<DirectoryEntry> list(Inode inode) throws IOException {
        counters.increment(Counters.Key.listDirectory);
        if (InodeMap.isRoot(inode)) {
            LOG.error("Can't list root inode");
            throw new IOException("Can't list root inode");
        }

        Optional<InodeMetadata> stat = inodeMap.stat(inode);
        if (!stat.isPresent()) {
            throw new NoEntException();
        }

        return inodeIndex.list(stat.get(), inode.exportIndex());
    }

    @Override
    public Inode mkdir(Inode parent, String path, Subject subject, int mode) throws IOException {
        counters.increment(Counters.Key.mkdir);
        Inode inode = create(parent, Stat.Type.DIRECTORY, path, subject, mode);
        link(inode, inode, ".", subject);
        link(inode, parent, "..", subject);
        return inode;
    }

    @Override
    public boolean move(Inode source, String oldName, Inode destination, String newName) throws IOException {
        counters.increment(Counters.Key.move);
        Optional<InodeMetadata> sourceMetadata = inodeMap.stat(source);
        if (!sourceMetadata.isPresent()) {
            throw new NoEntException();
        }

        Optional<InodeMetadata> linkMetadata = inodeIndex.lookup(source, oldName);
        if (!linkMetadata.isPresent()) {
            throw new NoEntException();
        }

        Optional<InodeMetadata> destinationMetadata = inodeMap.stat(source);
        if (!destinationMetadata.isPresent()) {
            throw new NoEntException();
        }

        if (!destinationMetadata.get().isDirectory()) {
            throw new NotDirException();
        }

        InodeMetadata updatedSource = sourceMetadata.get().withUpdatedTimestamps();

        InodeMetadata updatedLink = linkMetadata.get()
                .withoutLink(inodeMap.fileId(source), oldName)
                .withLink(inodeMap.fileId(destination), newName);

        InodeMetadata updatedDestination = destinationMetadata.get().withUpdatedTimestamps();

        inodeMap.update(source.exportIndex(), true, updatedSource, updatedLink, updatedDestination);
        inodeIndex.unlink(source.exportIndex(), InodeMetadata.fileId(source), oldName);
        inodeIndex.index(source.exportIndex(), true, updatedSource);
        inodeIndex.index(source.exportIndex(), true, updatedLink);
        inodeIndex.index(source.exportIndex(), true, updatedDestination);

        return true;
    }

    @Override
    public Inode parentOf(Inode inode) throws IOException {
        return null;
    }

    @Override
    public int read(Inode inode, byte[] data, long offset, int count) throws IOException {
        counters.increment(Counters.Key.read);
        Optional<InodeMetadata> target = inodeMap.stat(inode);
        if (!target.isPresent()) {
            throw new NoEntException();
        }

        String volumeName = exportResolver.volumeName(inode.exportIndex());
        String blobName = InodeMap.blobName(inode);
        try {
            int read = chunker.read(DOMAIN, volumeName, blobName, exportResolver.objectSize(volumeName), data, offset, count);
            counters.increment(Counters.Key.bytesRead, read);
            return read;
        } catch (Exception e) {
            LOG.error("Error reading blob " + blobName, e);
            throw new IOException(e);
        }
    }

    @Override
    public String readlink(Inode inode) throws IOException {
        counters.increment(Counters.Key.readLink);
        Optional<InodeMetadata> stat = inodeMap.stat(inode);
        if (!stat.isPresent()) {
            throw new NoEntException();
        }
        byte[] buf = new byte[(int) stat.get().getSize()];
        read(inode, buf, 0, buf.length);
        return new String(buf, "UTF-8");
    }

    @Override
    public void remove(Inode parentInode, String path) throws IOException {
        counters.increment(Counters.Key.remove);
        Optional<InodeMetadata> parent = inodeMap.stat(parentInode);
        if (!parent.isPresent()) {
            throw new NoEntException();
        }

        Optional<InodeMetadata> link = inodeIndex.lookup(parentInode, path);
        if (!link.isPresent()) {
            throw new NoEntException();
        }

        InodeMetadata updatedParent = parent.get().withUpdatedTimestamps();

        InodeMetadata updatedLink = link.get()
                .withoutLink(inodeMap.fileId(parentInode), path);


        inodeIndex.unlink(parentInode.exportIndex(), parent.get().getFileId(), path);
        if (updatedLink.refCount() == 0) {
            inodeMap.remove(updatedLink.asInode(parentInode.exportIndex()));
            inodeIndex.remove(parentInode.exportIndex(), updatedLink);
        } else {
            inodeMap.update(parentInode.exportIndex(), true, updatedLink);
            inodeIndex.index(parentInode.exportIndex(), true, updatedLink);
        }

        inodeMap.update(parentInode.exportIndex(), true, updatedParent);
        inodeIndex.index(parentInode.exportIndex(), true, updatedParent);
    }

    @Override
    public Inode symlink(Inode parent, String path, String link, Subject subject, int mode) throws IOException {
        counters.increment(Counters.Key.symlink);
        Optional<InodeMetadata> parentMd = inodeMap.stat(parent);
        if (!parentMd.isPresent()) {
            throw new NoEntException();
        }

        Inode inode = create(parent, Stat.Type.SYMLINK, path, subject, mode);
        byte[] linkValue = link.getBytes("UTF-8");
        write(inode, linkValue, 0, linkValue.length, StabilityLevel.FILE_SYNC);
        return inode;
    }

    @Override
    public WriteResult write(Inode inode, byte[] data, long offset, int count, StabilityLevel stabilityLevel) throws IOException {
        InodeMetadata updated = null;
        try {
            updated = inodeMap.write(inode, data, offset, count);
            inodeIndex.index(inode.exportIndex(), true, updated);
            counters.increment(Counters.Key.write);
            counters.increment(Counters.Key.bytesWritten, count);
            return new WriteResult(stabilityLevel, Math.max(data.length, count));
        } catch (IOException e) {
            if (updated != null) {
                LOG.error("Error writing inode " + updated.getFileId(), e);
            } else {
                LOG.error("Error writing inode", e);
            }

            throw e;
        }
    }

    @Override
    public void commit(Inode inode, long offset, int count) throws IOException {

    }

    @Override
    public Stat getattr(Inode inode) throws IOException {
        counters.increment(Counters.Key.getAttr);
        if (inodeMap.isRoot(inode)) {
            return InodeMap.ROOT_METADATA.asStat(inode.exportIndex());
        }

        Optional<InodeMetadata> stat = inodeMap.stat(inode);
        if (!stat.isPresent()) {
            LOG.error("getattr(): inode-" + InodeMetadata.fileId(inode) + " not found!");
            throw new NoEntException();
        }

        return stat.get().asStat(inode.exportIndex());
    }

    @Override
    public void setattr(Inode inode, Stat stat) throws IOException {
        counters.increment(Counters.Key.setAttr);
        Optional<InodeMetadata> metadata = inodeMap.stat(inode);
        if (!metadata.isPresent()) {
            throw new NoEntException();
        }

        InodeMetadata updated = metadata.get().update(stat);
        inodeMap.update(inode.exportIndex(), true, updated);
        inodeIndex.index(inode.exportIndex(), true, updated);
        LOG.debug("SETATTR " + exportResolver.volumeName(inode.exportIndex()) + "." + Joiner.on(",").join(updated.getLinks().values()) + ", mode=" + Integer.toOctalString(updated.getMode()));
    }

    @Override
    public nfsace4[] getAcl(Inode inode) throws IOException {
        if (inodeMap.isRoot(inode)) {
            return InodeMap.ROOT_METADATA.getNfsAces();
        }

        Optional<InodeMetadata> stat = inodeMap.stat(inode);
        if (!stat.isPresent()) {
            throw new NoEntException();
        }
        return stat.get().getNfsAces();
    }

    @Override
    public void setAcl(Inode inode, nfsace4[] acl) throws IOException {
        Optional<InodeMetadata> metadata = inodeMap.stat(inode);
        if (!metadata.isPresent()) {
            throw new NoEntException();
        }

        InodeMetadata updated = metadata.get().withNfsAces(acl);

        inodeMap.update(inode.exportIndex(), true, updated);
        inodeIndex.index(inode.exportIndex(), true, updated);
    }

    @Override
    public boolean hasIOLayout(Inode inode) throws IOException {
        return false;
    }

    @Override
    public AclCheckable getAclCheckable() {
        return this;
    }

    @Override
    public NfsIdMapping getIdMapper() {
        return idMap;
    }

    @Override
    public Access checkAcl(Subject subject, Inode inode, int accessMask) throws IOException {
        Optional<InodeMetadata> metadata = inodeMap.stat(inode);
        if (!metadata.isPresent()) {
            throw new NoEntException();
        }

        Stat stat = metadata.get().asStat(inode.exportIndex());
        nfsace4[] nfsAces = metadata.get().getNfsAces();
        List<ACE> aces = new ArrayList<>(nfsAces.length);
        for (nfsace4 nfsAce : nfsAces) {
            aces.add(AccessControl.makeAceFromNfsace4(nfsAce, idMap));
        }
        return AccessControl.check(subject, aces, stat.getUid(), stat.getGid(), accessMask);
    }

    private interface IoAction {
        public void execute() throws IOException;
    }

    private interface IoSupplier<T> {
        public T execute() throws IOException;
    }
}

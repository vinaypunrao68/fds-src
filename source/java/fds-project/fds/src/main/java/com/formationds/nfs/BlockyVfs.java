package com.formationds.nfs;

import com.formationds.xdi.AsyncAm;
import com.google.common.collect.Sets;
import org.apache.log4j.Logger;
import org.dcache.acl.ACE;
import org.dcache.acl.enums.AceFlags;
import org.dcache.acl.enums.AceType;
import org.dcache.acl.enums.Who;
import org.dcache.auth.GidPrincipal;
import org.dcache.auth.Subjects;
import org.dcache.auth.UidPrincipal;
import org.dcache.nfs.ChimeraNFSException;
import org.dcache.nfs.status.BadOwnerException;
import org.dcache.nfs.status.ExistException;
import org.dcache.nfs.status.NoEntException;
import org.dcache.nfs.status.NotDirException;
import org.dcache.nfs.v4.NfsIdMapping;
import org.dcache.nfs.v4.SimpleIdMap;
import org.dcache.nfs.v4.xdr.nfsace4;
import org.dcache.nfs.vfs.*;

import javax.security.auth.Subject;
import java.io.IOException;
import java.util.List;
import java.util.Optional;

import static org.dcache.nfs.v4.xdr.nfs4_prot.ACE4_INHERIT_ONLY_ACE;

public class BlockyVfs implements VirtualFileSystem, AclCheckable {
    public static final String DOMAIN = "nfs";
    private static final Logger LOG = Logger.getLogger(BlockyVfs.class);
    private InodeMap inodeMap;
    private InodeAllocator allocator;
    private InodeIndex inodeIndex;
    private ExportResolver exportResolver;
    private SimpleIdMap idMap;
    private Chunker chunker;

    public BlockyVfs(AsyncAm asyncAm, ExportResolver resolver) {
        AmIo amIo = new AmIo(asyncAm);
        inodeMap = new InodeMap(amIo, resolver);
        allocator = new InodeAllocator(amIo);
        this.exportResolver = resolver;
        inodeIndex = new SimpleInodeIndex(asyncAm, resolver);
        idMap = new SimpleIdMap();
        chunker = new Chunker(amIo);
    }

    @Override
    public int access(Inode inode, int mode) throws IOException {
        Optional<InodeMetadata> stat = inodeMap.stat(inode);
        if (!stat.isPresent()) {
            throw new NoEntException();
        }
        return mode;
    }

    @Override
    public Inode create(Inode parent, Stat.Type type, String name, Subject subject, int mode) throws IOException {
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
        InodeMetadata metadata = new InodeMetadata(type, subject, mode, allocator.allocate(volume), inodeMap.volumeId(parent))
                .withLink(inodeMap.fileId(parent), name);

        Inode inode = inodeMap.create(metadata, parent.exportIndex());
        InodeMetadata updatedParent = parentMetadata.get().withUpdatedTimestamps();
        inodeMap.update(inode.exportIndex(), updatedParent);
        inodeIndex.index(parent.exportIndex(), metadata, updatedParent);
        return inode;
    }


    @Override
    public FsStat getFsStat() throws IOException {
        return new FsStat(1024l * 1024l * 1024l * 1024l, 0, 0, 0);
    }

    @Override
    public Inode getRootInode() throws IOException {
        return InodeMap.ROOT;
    }

    @Override
    public Inode lookup(Inode parent, String path) throws IOException {
        if (InodeMap.isRoot(parent)) {
            String volumeName = path;
            if (!exportResolver.exists(volumeName)) {
                throw new NoEntException();
            }

            int exportId = (int) exportResolver.exportId(volumeName);
            Subject unixRootUser = new Subject(
                    true,
                    Sets.newHashSet(new UidPrincipal(0), new GidPrincipal(0, true)),
                    Sets.newHashSet(),
                    Sets.newHashSet());

            InodeMetadata inodeMetadata = new InodeMetadata(Stat.Type.DIRECTORY, unixRootUser, 0755, InodeAllocator.START_VALUE, exportId)
                    .withUpdatedAtime()
                    .withUpdatedCtime()
                    .withUpdatedMtime()
                    .withLink(InodeAllocator.START_VALUE, ".");

            Optional<InodeMetadata> statResult = inodeMap.stat(inodeMetadata.asInode(exportId));
            if (!statResult.isPresent()) {
                LOG.debug("Creating export root for volume " + path);
                Inode inode = inodeMap.create(inodeMetadata, exportId);
                inodeIndex.index(exportId, inodeMetadata);
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

        inodeMap.update(parent.exportIndex(), updatedParentMetadata, updatedLinkMetadata);
        inodeIndex.index(parent.exportIndex(), updatedParentMetadata, updatedLinkMetadata);
        return link;
    }

    @Override
    public List<DirectoryEntry> list(Inode inode) throws IOException {
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
        Inode inode = create(parent, Stat.Type.DIRECTORY, path, subject, mode);
        link(inode, inode, ".", subject);
        link(inode, parent, "..", subject);
        return inode;
    }

    @Override
    public boolean move(Inode source, String oldName, Inode destination, String newName) throws IOException {
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
                .withoutLink(inodeMap.fileId(source))
                .withLink(inodeMap.fileId(destination), newName);

        InodeMetadata updatedDestination = destinationMetadata.get().withUpdatedTimestamps();

        inodeMap.update(source.exportIndex(), updatedSource, updatedLink, updatedDestination);
        inodeIndex.unlink(source, oldName);
        inodeIndex.index(source.exportIndex(), updatedSource, updatedLink, updatedDestination);

        return true;
    }

    @Override
    public Inode parentOf(Inode inode) throws IOException {
        return null;
    }

    @Override
    public int read(Inode inode, byte[] data, long offset, int count) throws IOException {
        Optional<InodeMetadata> target = inodeMap.stat(inode);
        if (!target.isPresent()) {
            throw new NoEntException();
        }

        String volumeName = exportResolver.volumeName(inode.exportIndex());
        String blobName = InodeMap.blobName(inode);
        try {
            return chunker.read(DOMAIN, volumeName, blobName, exportResolver.objectSize(volumeName), data, offset, count);
        } catch (Exception e) {
            LOG.error("Error reading blob " + blobName, e);
            throw new IOException(e);
        }
    }

    @Override
    public String readlink(Inode inode) throws IOException {
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
                .withoutLink(inodeMap.fileId(parentInode));


        inodeIndex.unlink(parentInode, path);
        if (updatedLink.refCount() == 0) {
            inodeMap.remove(updatedLink.asInode(parentInode.exportIndex()));
            inodeIndex.remove(parentInode.exportIndex(), updatedLink);
        } else {
            inodeMap.update(parentInode.exportIndex(), updatedLink);
            inodeIndex.index(parentInode.exportIndex(), updatedLink);
        }

        inodeMap.update(parentInode.exportIndex(), updatedParent);
        inodeIndex.index(parentInode.exportIndex(), updatedParent);
    }

    @Override
    public Inode symlink(Inode parent, String path, String link, Subject subject, int mode) throws IOException {
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
        InodeMetadata updated = inodeMap.write(inode, data, offset, count);
        inodeIndex.index(inode.exportIndex(), updated);
        return new WriteResult(stabilityLevel, Math.max(data.length, count));
    }

    @Override
    public void commit(Inode inode, long offset, int count) throws IOException {

    }

    @Override
    public Stat getattr(Inode inode) throws IOException {
        if (inodeMap.isRoot(inode)) {
            return InodeMap.ROOT_METADATA.asStat(inode.exportIndex());
        }

        Optional<InodeMetadata> stat = inodeMap.stat(inode);
        if (!stat.isPresent()) {
            throw new NoEntException();
        }
        return stat.get().asStat(inode.exportIndex());
    }

    @Override
    public void setattr(Inode inode, Stat stat) throws IOException {
        Optional<InodeMetadata> metadata = inodeMap.stat(inode);
        if (!metadata.isPresent()) {
            throw new NoEntException();
        }

        InodeMetadata updated = metadata.get().update(stat);

        inodeMap.update(inode.exportIndex(), updated);
        inodeIndex.index(inode.exportIndex(), updated);
    }

    @Override
    public nfsace4[] getAcl(Inode inode) throws IOException {
//        if (inodeMap.isRoot(inode)) {
//            return InodeMap.ROOT_METADATA.getNfsAces();
//        }
//
//        Optional<InodeMetadata> stat = inodeMap.stat(inode);
//        if (!stat.isPresent()) {
//            throw new NoEntException();
//        }
//        return stat.get().getNfsAces();
        return new nfsace4[0];
    }

    @Override
    public void setAcl(Inode inode, nfsace4[] acl) throws IOException {
//        Optional<InodeMetadata> metadata = inodeMap.stat(inode);
//        if (!metadata.isPresent()) {
//            throw new NoEntException();
//        }
//
//        InodeMetadata updated = metadata.get().withNfsAces(acl);
//
//        inodeMap.update(updated);
//        inodeIndex.index(updated);
    }

    @Override
    public boolean hasIOLayout(Inode inode) throws IOException {
        return false;
    }

    @Override
    public AclCheckable getAclCheckable() {
        return null;
    }

    private static ACE valueOf(nfsace4 ace, NfsIdMapping idMapping) throws BadOwnerException {
        String principal = ace.who.toString();
        int type = ace.type.value.value;
        int flags = ace.flag.value.value;
        int mask = ace.access_mask.value.value;

        int id = -1;
        Who who = Who.fromAbbreviation(principal);
        if (who == null) {
            // not a special pricipal
            boolean isGroup = AceFlags.IDENTIFIER_GROUP.matches(flags);
            if (isGroup) {
                who = Who.GROUP;
                id = idMapping.principalToGid(principal);
            } else {
                who = Who.USER;
                id = idMapping.principalToUid(principal);
            }
        }
        return new ACE(AceType.valueOf(type), flags, mask, who, id, ACE.DEFAULT_ADDRESS_MSK);
    }

    @Override
    public Access checkAcl(Subject subject, Inode inode, int access) throws ChimeraNFSException, IOException {
        Optional<InodeMetadata> metadata = inodeMap.stat(inode);
        if (!metadata.isPresent()) {
            throw new NoEntException();
        }

        nfsace4[] nfsAces = metadata.get().getNfsAces();
        for (nfsace4 ace4 : nfsAces) {
            ACE ace = valueOf(ace4, idMap);
            int flag = ace.getFlags();
            if ((flag & ACE4_INHERIT_ONLY_ACE) != 0) {
                continue;
            }

            if ((ace.getType() != AceType.ACCESS_ALLOWED_ACE_TYPE) && (ace.getType() != AceType.ACCESS_DENIED_ACE_TYPE)) {
                continue;
            }

            int ace_mask = ace.getAccessMsk();
            if ((ace_mask & access) == 0) {
                continue;
            }

            Who who = ace.getWho();

            if ((who == Who.EVERYONE)
                    || (who == Who.OWNER & Subjects.hasUid(subject, metadata.get().getUid()))
                    || (who == Who.OWNER_GROUP & Subjects.hasGid(subject, metadata.get().getGid()))
                    || (who == Who.GROUP & Subjects.hasGid(subject, ace.getWhoID()))
                    || (who == Who.USER & Subjects.hasUid(subject, ace.getWhoID()))) {

                if (ace.getType() == AceType.ACCESS_DENIED_ACE_TYPE) {
                    return Access.DENY;
                } else {
                    return Access.ALLOW;
                }
            }
        }

        return Access.UNDEFINED;
    }

    @Override
    public NfsIdMapping getIdMapper() {
        return idMap;
    }
}

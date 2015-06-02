package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import com.google.common.collect.Sets;
import org.apache.log4j.Logger;
import org.dcache.auth.GidPrincipal;
import org.dcache.auth.UidPrincipal;
import org.dcache.nfs.status.ExistException;
import org.dcache.nfs.status.NoEntException;
import org.dcache.nfs.status.NotDirException;
import org.dcache.nfs.v4.NfsIdMapping;
import org.dcache.nfs.v4.SimpleIdMap;
import org.dcache.nfs.v4.xdr.nfsace4;
import org.dcache.nfs.vfs.*;

import javax.security.auth.Subject;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Optional;
import java.util.UUID;
import java.util.stream.Collectors;

import static com.formationds.hadoop.FdsFileSystem.*;

public class AmVfs implements VirtualFileSystem {
    private static final Logger LOG = Logger.getLogger(AmVfs.class);
    private AsyncAm asyncAm;
    private static final String DOMAIN = "nfs";

    public AmVfs(AsyncAm asyncAm) {
        this.asyncAm = asyncAm;
    }

    @Override
    public int access(Inode inode, int mode) throws IOException {
        NfsPath path = new NfsPath(inode);
        Optional<NfsAttributes> attributes = load(path);
        if (!attributes.isPresent()) {
            throw new NoEntException();
        }
        return mode;
    }

    @Override
    public Inode create(Inode inode, Stat.Type type, String name, Subject subject, int mode) throws IOException {
        NfsPath parent = new NfsPath(inode);
        if (!load(parent).isPresent()) {
            throw new NoEntException();
        }

        NfsPath childPath = new NfsPath(parent, name);
        if (load(childPath).isPresent()) {
            throw new ExistException();
        }

        return createInode(type, subject, mode, childPath);
    }

    @Override
    public FsStat getFsStat() throws IOException {
        return new FsStat(1024 * 1024 * 10, 0, 0, 0);
    }

    @Override
    public Inode getRootInode() throws IOException {
        return new Inode(new FileHandle(0, 0, Stat.S_IFDIR, new byte[0]));
    }

    @Override
    public Inode lookup(Inode inode, String name) throws IOException {
        NfsPath parent = new NfsPath(inode);
        NfsPath child = new NfsPath(parent, name);
        Optional<NfsAttributes> attributes = load(child);
        if (!attributes.isPresent()) {
            throw new NoEntException();
        }
        return child.asInode(attributes.get().getType());
    }

    @Override
    public Inode link(Inode inode, Inode inode1, String s, Subject subject) throws IOException {
        throw new RuntimeException();
    }

    @Override
    public List<DirectoryEntry> list(Inode inode) throws IOException {
        NfsPath path = new NfsPath(inode);
        Optional<NfsAttributes> attributes = load(path);
        if (!attributes.isPresent()) {
            throw new NoEntException();
        }
        if (!attributes.get().getType().equals(Stat.Type.DIRECTORY)) {
            throw new NotDirException();
        }

        StringBuilder filter = new StringBuilder();
        filter.append(path.blobName());

        if (!path.blobName().endsWith("/")) {
            filter.append("/");
        }
        filter.append("[^/]+$");

        try {
            List<BlobDescriptor> blobDescriptors = unwindExceptions(() ->
                    asyncAm.volumeContents(DOMAIN,
                            path.getVolume(),
                            Integer.MAX_VALUE,
                            0,
                            filter.toString(),
                            BlobListOrder.UNSPECIFIED,
                            false).
                            get());
            return blobDescriptors
                    .stream()
                    .map(bd -> {
                        NfsPath childPath = new NfsPath(path, bd.getName());
                        NfsAttributes childAttributes = new NfsAttributes(bd.getMetadata());
                        return new DirectoryEntry(childPath.fileName(), inode, childAttributes.asStat());
                    })
                    .collect(Collectors.toList());
        } catch (Exception e) {
            throw new IOException(e);
        }
    }

    @Override
    public Inode mkdir(Inode parentInode, String name, Subject subject, int mode) throws IOException {
        NfsPath parent = new NfsPath(parentInode);
        NfsPath path = new NfsPath(parent, name);
        if (load(path).isPresent()) {
            throw new ExistException();
        }

        return createInode(Stat.Type.DIRECTORY, subject, mode, path);
    }

    @Override
    public boolean move(Inode inode, String s, Inode inode1, String s1) throws IOException {
        return false;
    }

    @Override
    public Inode parentOf(Inode inode) throws IOException {
        return null;
    }

    @Override
    public int read(Inode inode, byte[] bytes, long l, int i) throws IOException {
        return 0;
    }

    @Override
    public String readlink(Inode inode) throws IOException {
        return null;
    }

    @Override
    public void remove(Inode inode, String s) throws IOException {

    }

    @Override
    public Inode symlink(Inode inode, String s, String s1, Subject subject, int i) throws IOException {
        return null;
    }

    @Override
    public WriteResult write(Inode inode, byte[] bytes, long l, int i, StabilityLevel stabilityLevel) throws IOException {
        return null;
    }

    @Override
    public void commit(Inode inode, long l, int i) throws IOException {

    }

    @Override
    public Stat getattr(Inode inode) throws IOException {
        NfsPath path = new NfsPath(inode);
        Optional<NfsAttributes> attributes = load(path);
        if (!attributes.isPresent()) {
            throw new NoEntException();
        }
        return attributes.get().asStat();
    }

    @Override
    public void setattr(Inode inode, Stat stat) throws IOException {
        System.out.println(stat);
    }

    @Override
    public nfsace4[] getAcl(Inode inode) throws IOException {
        return new nfsace4[0];
    }

    @Override
    public void setAcl(Inode inode, nfsace4[] nfsace4s) throws IOException {

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

    public void logCreationError(NfsPath path, Exception e) {
        LOG.error("Error creating blob [volume:" + path.getVolume() + "]" + path.blobName(), e);
    }

    private Optional<NfsAttributes> load(NfsPath path) throws IOException {
        if (path.isRoot()) {
            return Optional.of(new NfsAttributes(Stat.Type.DIRECTORY, new Subject(), Stat.S_IFDIR | 0755, 0));
        }
        try {
            BlobDescriptor blobDescriptor = unwindExceptions(() -> asyncAm.statBlob(DOMAIN, path.getVolume(), path.blobName()).get());
            return Optional.of(new NfsAttributes(blobDescriptor.getMetadata()));
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                if (path.blobName().equals("/")) {
                    createExportRoot(path.getVolume());
                    return load(path);
                }
                return Optional.empty();
            } else {
                LOG.error("Error accessing [volume:" + path.getVolume() + "]" + path.blobName(), e);
                throw new IOException(e);
            }
        } catch (Exception e) {
            LOG.error("Error accessing [volume:" + path.getVolume() + "]" + path.blobName(), e);
            throw new IOException(e);
        }
    }


    public Inode createInode(Stat.Type type, Subject subject, int mode, NfsPath path) throws IOException {
        Inode childInode = path.asInode(type);
        NfsAttributes attributes = new NfsAttributes(type, subject, mode, nextFileId());

        try {
            unwindExceptions(() -> asyncAm.updateBlobOnce(DOMAIN,
                    path.getVolume(),
                    path.blobName(),
                    1,
                    ByteBuffer.allocate(0),
                    0,
                    new ObjectOffset(),
                    attributes.asMetadata())
                    .get());
        } catch (Exception e) {
            logCreationError(path, e);
            throw new IOException(e);
        }
        return childInode;
    }

    private void createExportRoot(String volume) throws IOException {
        Subject unixRootUser = new Subject(
                true,
                Sets.newHashSet(new UidPrincipal(0), new GidPrincipal(0, true)),
                Sets.newHashSet(),
                Sets.newHashSet());

         createInode(Stat.Type.DIRECTORY, unixRootUser, 0755, new NfsPath(volume, "/"));
    }

    private long nextFileId() {
        return Math.abs(UUID.randomUUID().getLeastSignificantBits());
    }
}

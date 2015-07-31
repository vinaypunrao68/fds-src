package com.formationds.nfs;

import com.formationds.apis.TxDescriptor;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import org.apache.log4j.Logger;
import org.dcache.nfs.vfs.FileHandle;
import org.dcache.nfs.vfs.Inode;
import org.dcache.nfs.vfs.Stat;

import javax.security.auth.Subject;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Optional;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class InodeMap {
    private static final Logger LOG = Logger.getLogger(InodeMap.class);
    private final AsyncAm asyncAm;
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

    public InodeMap(AsyncAm asyncAm, ExportResolver exportResolver) {
        this.asyncAm = asyncAm;
        this.exportResolver = exportResolver;
    }


    public Optional<InodeMetadata> stat(Inode inode) throws IOException {
        if (isRoot(inode)) {
            return Optional.of(ROOT_METADATA);
        }

        String blobName = blobName(inode);
        String volumeName = volumeName(inode);

        try {
            BlobDescriptor blobDescriptor = unwindExceptions(() -> asyncAm.statBlob(BlockyVfs.DOMAIN, volumeName, blobName).get());
            return Optional.of(new InodeMetadata(blobDescriptor));
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                return Optional.empty();
            } else {
                throw new IOException(e);
            }
        } catch (Exception e) {
            throw new IOException(e);
        }
    }

    public static String blobName(Inode inode) {
        return "inode-" + InodeMetadata.fileId(inode);
    }

    public Inode create(InodeMetadata metadata) throws IOException {
        return doUpdate(metadata, 1);
    }

    private Inode doUpdate(InodeMetadata metadata, int mode) throws IOException {
        String volume = exportResolver.volumeName((int) metadata.getVolumeId());
        String blobName = blobName(metadata.asInode());
        try {
            TxDescriptor desc = unwindExceptions(() -> {
                TxDescriptor tx = asyncAm.startBlobTx(BlockyVfs.DOMAIN, volume, blobName, mode).get();
                asyncAm.updateMetadata(BlockyVfs.DOMAIN, volume, blobName, tx, metadata.asMap()).get();
                asyncAm.commitBlobTx(BlockyVfs.DOMAIN, volume, blobName, tx).get();
                return tx;
            });
        } catch (Exception e) {
            LOG.error("error creating " + blobName + " in volume " + volume, e);
            throw new IOException(e);
        }
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
            doUpdate(entry, 0);
        }
    }

    public void remove(Inode inode) throws IOException {
        String blobName = blobName(inode);
        String volumeName = volumeName(inode);

        try {
            unwindExceptions(() -> asyncAm.deleteBlob(BlockyVfs.DOMAIN, volumeName, blobName).get());
        } catch (Exception e) {
            LOG.debug("error creating " + blobName + " in volume " + volumeName, e);
            throw new IOException(e);
        }
    }

    public static boolean isRoot(Inode inode) {
        return InodeMetadata.fileId(inode) == Long.MAX_VALUE;
    }
}

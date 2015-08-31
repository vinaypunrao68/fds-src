package com.formationds.index;

import com.formationds.hadoop.FdsInputStream;
import com.formationds.hadoop.FdsOutputStream;
import com.formationds.hadoop.OwnerGroupInfo;
import com.formationds.nfs.BlockyVfs;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.PatternSemantics;
import com.formationds.xdi.AsyncAm;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.log4j.Logger;
import org.apache.lucene.store.*;

import java.io.IOException;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class FdsLuceneDirectory extends Directory {
    private static final Logger LOG = Logger.getLogger(FdsLuceneDirectory.class);
    public static final String INDEX_FILE_PREFIX = "index-";
    public static final OwnerGroupInfo OWNER = new OwnerGroupInfo("fds", "fds");
    private final AsyncAm asyncAm;
    private final String volume;
    private final int objectSize;
    private final ConcurrentHashMap<String, Lock> locks;
    private final Set<String> indexFiles;

    public FdsLuceneDirectory(AsyncAm asyncAm, String volume, int objectSize) throws IOException {
        this.asyncAm = asyncAm;
        this.volume = volume;
        this.objectSize = objectSize;
        locks = new ConcurrentHashMap<>();
        indexFiles = new HashSet<>();
        try {
            List<BlobDescriptor> blobs = unwindExceptions(() -> asyncAm.volumeContents(BlockyVfs.DOMAIN, volume, Integer.MAX_VALUE, 0, "^index-.*", PatternSemantics.PCRE, BlobListOrder.UNSPECIFIED, false).get());
            for (BlobDescriptor blob : blobs) {
                String indexFile = blob.getName().replaceAll("^index-", "");
                indexFiles.add(indexFile);
            }
        } catch (Exception e) {
            LOG.error("Error listing blobs for volume " + volume, e);
            throw new IOException(e);
        }
    }

    @Override
    public String[] listAll() throws IOException {
        return indexFiles.toArray(new String[indexFiles.size()]);
    }

    @Override
    public void deleteFile(String s) throws IOException {
        LOG.debug("delete " + s);
        try {
            unwindExceptions(() -> asyncAm.deleteBlob(BlockyVfs.DOMAIN, volume, blobName(s)).get());
        } catch (Exception e) {
            LOG.error("Error deleting index file " + blobName(s) + " in volume " + volume);
            throw new IOException(e);
        }
        indexFiles.remove(s);
    }

    @Override
    public long fileLength(String s) throws IOException {
        String indexFile = blobName(s);
        try {
            return unwindExceptions(() -> asyncAm.statBlob(BlockyVfs.DOMAIN, volume, indexFile).get()).getByteCount();
        } catch (Exception e) {
            LOG.error("Error getting file length for " + indexFile, e);
            throw new IOException(e);
        }
    }

    @Override
    public IndexOutput createOutput(String fileName, IOContext ioContext) throws IOException {
        LOG.debug("Create output " + fileName);
        indexFiles.add(fileName);
        FdsOutputStream out = FdsOutputStream.openNew(asyncAm, BlockyVfs.DOMAIN, volume, blobName(fileName), objectSize, OWNER);
        return new OutputStreamIndexOutput(fileName, out, objectSize);
    }

    @Override
    public void sync(Collection<String> collection) throws IOException {
    }

    @Override
    public void renameFile(String from, String to) throws IOException {
        LOG.debug("Rename file " + from + " " + to);
        FdsInputStream in = new FdsInputStream(asyncAm, BlockyVfs.DOMAIN, volume, blobName(from), objectSize);
        FdsOutputStream out = FdsOutputStream.openNew(asyncAm, BlockyVfs.DOMAIN, volume, blobName(to), objectSize, OWNER);
        byte[] buf = new byte[objectSize];

        for (int read = in.read(buf); read != -1; read = in.read(buf)) {
            out.write(buf, 0, read);
        }
        in.close();
        out.close();
        indexFiles.remove(from);
        try {
            unwindExceptions(() -> asyncAm.deleteBlob(BlockyVfs.DOMAIN, volume, blobName(from)).get());
        } catch (Exception e) {
            LOG.error("Error deleting " + from + " in volume " + volume, e);
            throw new IOException(e);
        }
        indexFiles.add(to);
    }

    private String blobName(String indexFile) {
        return INDEX_FILE_PREFIX + indexFile;
    }

    @Override
    public IndexInput openInput(String indexFile, IOContext ioContext) throws IOException {
        LOG.debug("open input " + indexFile);
        return new FdsIndexInput(indexFile);
    }

    @Override
    public Lock makeLock(String s) {
        return locks.compute(s, (k, v) -> {
            if (v == null) {
                v = new MemoryLock();
            }

            return v;
        });
    }

    @Override
    public void close() throws IOException {
    }

    private class FdsIndexInput extends IndexInput {
        FdsInputStream in;
        FSDataInputStream stream;
        private final long length;
        private final long offset;

        public FdsIndexInput(String indexFileName) throws IOException {
            super(indexFileName);
            in = new FdsInputStream(asyncAm, BlockyVfs.DOMAIN, volume, blobName(indexFileName), objectSize);
            stream = new FSDataInputStream(in);
            this.offset = 0;
            try {
                this.length = in.length();
            } catch (Exception e) {
                throw new IOException(e);
            }
        }

        public FdsIndexInput(String resourceName, String blobName, long offset, long length) throws IOException {
            super(resourceName);
            this.in = new FdsInputStream(asyncAm, BlockyVfs.DOMAIN, volume, blobName, objectSize);
            in.seek(offset);
            stream = new FSDataInputStream(in);
            this.offset = offset;
            this.length = length;
        }

        @Override
        public IndexInput clone() {
            FdsIndexInput indexInput = null;
            try {
                indexInput = new FdsIndexInput(toString(), in.getBlobName(), offset, length());
                indexInput.seek(this.getFilePointer());
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
            return indexInput;
        }

        @Override
        public void close() throws IOException {
            stream.close();
        }

        @Override
        public long getFilePointer() {
            try {
                return stream.getPos() - offset;
            } catch (IOException e) {
                LOG.error("Error getting position for " + in.getBlobName() + " in volume " + volume, e);
                throw new RuntimeException(e);
            }
        }

        @Override
        public void seek(long l) throws IOException {
            stream.seek(l + offset);
        }

        @Override
        public long length() {
            return length;
        }

        @Override
        public IndexInput slice(String name, long offset, long length) throws IOException {
            return new FdsIndexInput(name, in.getBlobName(), offset + this.offset, length);
        }

        @Override
        public byte readByte() throws IOException {
            return stream.readByte();
        }

        @Override
        public void readBytes(byte[] bytes, int offset, int length) throws IOException {
            stream.read(bytes, offset, length);
        }
    }
}

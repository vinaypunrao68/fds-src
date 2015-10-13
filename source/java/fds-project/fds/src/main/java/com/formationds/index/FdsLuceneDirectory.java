package com.formationds.index;

import com.formationds.hadoop.OwnerGroupInfo;
import com.formationds.nfs.*;
import com.formationds.xdi.AsyncAm;
import org.apache.log4j.Logger;
import org.apache.lucene.store.*;
import org.junit.Ignore;

import java.io.IOException;
import java.io.OutputStream;
import java.util.Collection;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

@Ignore
public class FdsLuceneDirectory extends Directory {
    private static final Logger LOG = Logger.getLogger(FdsLuceneDirectory.class);
    public static final String INDEX_FILE_PREFIX = "index-";
    public static final OwnerGroupInfo OWNER = new OwnerGroupInfo("fds", "fds");
    public static final String SIZE = "SIZE";
    public static final String NAME = "NAME";
    private final String volume;
    private final int objectSize;
    private final ConcurrentHashMap<String, Lock> locks;
    private final TransactionalIo io;
    private final Chunker chunker;

    public FdsLuceneDirectory(AsyncAm asyncAm, String volume, int objectSize) throws IOException {
        this(new AmOps(asyncAm, new Counters()), volume, objectSize);
    }

    public FdsLuceneDirectory(IoOps ops, String volume, int objectSize) {
        this.volume = volume;
        this.objectSize = objectSize;
        locks = new ConcurrentHashMap<>();
        io = new TransactionalIo(ops);
        chunker = new Chunker(io);
    }

    @Override
    public String[] listAll() throws IOException {
        return io.scan(BlockyVfs.DOMAIN, volume, INDEX_FILE_PREFIX,
                metadata -> metadata.get().get(NAME)).toArray(new String[0]);
    }

    @Override
    public void deleteFile(String s) throws IOException {
        io.deleteBlob(BlockyVfs.DOMAIN, volume, blobName(s));
    }

    @Override
    public long fileLength(String s) throws IOException {
        String indexFile = blobName(s);
        return io.mapMetadata(BlockyVfs.DOMAIN, volume, indexFile, metadata -> {
            if (!metadata.isPresent()) {
                return 0l;
            }

            return Long.parseLong(metadata.get().get(SIZE));
        });
    }

    @Override
    public IndexOutput createOutput(String fileName, IOContext ioContext) throws IOException {
        String blobName = blobName(fileName);
        boolean exists = io.mapMetadata(BlockyVfs.DOMAIN, volume, blobName, metadata -> metadata.isPresent());
        if (exists) {
            io.deleteBlob(BlockyVfs.DOMAIN, volume, blobName);
        }

        OutputStream out = new ChunkedOutputStream(io, BlockyVfs.DOMAIN, volume, blobName, objectSize);
        return new OutputStreamIndexOutput(fileName, out, objectSize);
    }

    @Override
    public void sync(Collection<String> collection) throws IOException {
    }

    @Override
    public void renameFile(String from, String to) throws IOException {
        LOG.debug("Rename file " + from + " " + to);
        io.mutateMetadata(BlockyVfs.DOMAIN, volume, blobName(from), false, new MetadataMutator<Void>() {
            @Override
            public Void mutate(Map<String, String> metadata) throws IOException {
                metadata.put(NAME, to);
                return null;
            }
        });
        io.renameBlob(BlockyVfs.DOMAIN, volume, blobName(from), blobName(to));
    }

    private String blobName(String indexFile) {
        return INDEX_FILE_PREFIX + indexFile;
    }

    @Override
    public IndexInput openInput(String indexFile, IOContext ioContext) throws IOException {
        LOG.debug("open input " + indexFile);
        return new FdsIndexInput(io, indexFile, BlockyVfs.DOMAIN, volume, blobName(indexFile), objectSize);
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

    public static class FdsIndexInput extends IndexInput {
        private String resourceName;
        private Chunker chunker;
        private final long length;
        private final long offset;
        private long position;
        private String domain;
        private String volume;
        private String blobName;
        private int objectSize;
        private TransactionalIo io;

        public FdsIndexInput(TransactionalIo io, String resourceName, String domain, String volume, String blobName, int objectSize) throws IOException {
            super(resourceName);
            this.io = io;
            this.resourceName = resourceName;
            chunker = new Chunker(io);
            this.domain = domain;
            this.volume = volume;
            this.blobName = blobName;
            this.objectSize = objectSize;
            this.offset = 0;
            this.position = 0;
            this.length = io.mapMetadata(BlockyVfs.DOMAIN, volume, blobName, om -> Long.parseLong(om.get().get(SIZE)));
        }

        private FdsIndexInput(TransactionalIo io, String resourceName, String domain, String volume, String blobName, int objectSize, long offset, long length) {
            super(resourceName);
            this.io = io;
            this.resourceName = resourceName;
            chunker = new Chunker(io);
            this.domain = domain;
            this.volume = volume;
            this.blobName = blobName;
            this.objectSize = objectSize;
            this.offset = offset;
            this.position = offset;
            this.length = length;
        }

        @Override
        public IndexInput clone() {
            FdsIndexInput indexInput = new FdsIndexInput(io, toString(), domain, volume, blobName, objectSize, offset, length);
            indexInput.position = this.position;
            return indexInput;
        }

        @Override
        public void close() throws IOException {
        }

        @Override
        public long getFilePointer() {
            return position - offset;
        }

        @Override
        public void seek(long l) throws IOException {
            position = l + offset;
        }

        @Override
        public long length() {
            return length;
        }

        @Override
        public IndexInput slice(String name, long offset, long length) throws IOException {
            return new FdsIndexInput(io, name, domain, volume, blobName, objectSize, offset + this.offset, length);
        }

        @Override
        public byte readByte() throws IOException {
            byte[] buf = new byte[1];
            readBytes(buf, 0, 1);
            return buf[0];
        }

        @Override
        public void readBytes(byte[] bytes, int offset, int length) throws IOException {
            byte[] buf = new byte[length];
            chunker.read(BlockyVfs.DOMAIN, volume, blobName, objectSize, buf, position, length);
            position += length;
            System.arraycopy(buf, 0, bytes, offset, length);
        }
    }

}

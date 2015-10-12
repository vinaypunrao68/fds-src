package com.formationds.index;

import com.formationds.hadoop.OwnerGroupInfo;
import com.formationds.nfs.*;
import com.formationds.xdi.AsyncAm;
import org.apache.log4j.Logger;
import org.apache.lucene.store.*;

import java.io.IOException;
import java.io.OutputStream;
import java.util.Collection;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

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
        Counters counters = new Counters();
        DeferredIoOps deferredIo = new DeferredIoOps(ops, counters);
        deferredIo.start();
        io = new TransactionalIo(deferredIo);
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
        OutputStream out = new OutputStream() {
            long position = io.mutateMetadata(BlockyVfs.DOMAIN, volume, blobName(fileName), false, metadata -> {
                if (metadata.size() == 0) {
                    metadata.put(NAME, fileName);
                    metadata.put(SIZE, Long.toString(0l));
                }
                return Long.parseLong(metadata.get(SIZE));
            });

            @Override
            public void write(int b) throws IOException {
                write(new byte[]{(byte) b}, 0, 1);
            }

            @Override
            public void write(byte[] b, int off, int len) throws IOException {
                byte[] buf = new byte[len];
                System.arraycopy(b, off, buf, 0, len);
                chunker.write(BlockyVfs.DOMAIN, volume, blobName(fileName), objectSize, buf, position, len, metadata -> {
                    position += len;
                    metadata.put(SIZE, Long.toString(position));
                    return null;
                });
            }
        };
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
        private final long length;
        private final long offset;
        private long position;
        private String blobName;

        public FdsIndexInput(String indexFileName) throws IOException {
            super(indexFileName);
            this.blobName = blobName(indexFileName);
            this.offset = 0;
            this.position = 0;
            this.length = io.mapMetadata(BlockyVfs.DOMAIN, volume, blobName, om -> Long.parseLong(om.get().get(SIZE)));
        }

        public FdsIndexInput(String resourceName, String blobName, long offset, long length) {
            super(resourceName);
            this.blobName = blobName;
            this.offset = offset;
            this.length = length;
            this.position = offset;
        }

        @Override
        public IndexInput clone() {
            FdsIndexInput indexInput = new FdsIndexInput(toString(), blobName, offset, length());
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
            return new FdsIndexInput(name, blobName, offset + this.offset, length);
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
            chunker.read(BlockyVfs.DOMAIN, volume, blobName, objectSize, buf, position + this.offset, length);
            position += length;
            System.arraycopy(buf, 0, bytes, offset, length);
        }
    }
}

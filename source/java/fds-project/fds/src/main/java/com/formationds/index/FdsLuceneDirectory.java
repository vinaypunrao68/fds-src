package com.formationds.index;

import com.formationds.hadoop.OwnerGroupInfo;
import com.formationds.nfs.IoOps;
import com.formationds.nfs.TransactionalIo;
import org.apache.log4j.Logger;
import org.apache.lucene.store.*;
import org.junit.Ignore;

import java.io.IOException;
import java.io.OutputStream;
import java.util.Collection;
import java.util.concurrent.ConcurrentHashMap;

@Ignore
public class FdsLuceneDirectory extends Directory {
    private static final Logger LOG = Logger.getLogger(FdsLuceneDirectory.class);
    public static final String INDEX_FILE_PREFIX = "index-";
    public static final OwnerGroupInfo OWNER = new OwnerGroupInfo("fds", "fds");
    public static final String SIZE = "SIZE";
    public static final String LUCENE_RESOURCE_NAME = "NAME";
    private String domain;
    private final String volume;
    private final int objectSize;
    private final ConcurrentHashMap<String, Lock> locks;
    private final TransactionalIo io;


    public FdsLuceneDirectory(IoOps ops, String domain, String volume, int objectSize) {
        this.domain = domain;
        this.volume = volume;
        this.objectSize = objectSize;
        locks = new ConcurrentHashMap<>();
        io = new TransactionalIo(ops);
    }

    @Override
    public String[] listAll() throws IOException {
        return io.scan(domain, volume, INDEX_FILE_PREFIX,
                metadata -> metadata.get().get(LUCENE_RESOURCE_NAME)).toArray(new String[0]);
    }

    @Override
    public void deleteFile(String s) throws IOException {
        io.deleteBlob(domain, volume, blobName(s));
    }

    @Override
    public long fileLength(String s) throws IOException {
        String indexFile = blobName(s);
        return io.mapMetadata(domain, volume, indexFile, metadata -> {
            if (!metadata.isPresent()) {
                return 0l;
            }

            return Long.parseLong(metadata.get().get(SIZE));
        });
    }

    @Override
    public IndexOutput createOutput(String fileName, IOContext ioContext) throws IOException {
        String blobName = blobName(fileName);
        boolean exists = io.mapMetadata(domain, volume, blobName, metadata -> metadata.isPresent());
        if (exists) {
            io.deleteBlob(domain, volume, blobName);
        }

        OutputStream out = new ChunkedOutputStream(io, fileName, domain, volume, blobName, objectSize);
        return new OutputStreamIndexOutput(fileName, out, objectSize);
    }

    @Override
    public void sync(Collection<String> collection) throws IOException {
    }

    @Override
    public void renameFile(String from, String to) throws IOException {
        LOG.debug("Rename file " + from + " " + to);
        io.mutateMetadata(domain, volume, blobName(from), false, metadata -> {
            metadata.put(LUCENE_RESOURCE_NAME, to);
            return null;
        });
        io.renameBlob(domain, volume, blobName(from), blobName(to));
    }

    private String blobName(String indexFile) {
        return INDEX_FILE_PREFIX + indexFile;
    }

    @Override
    public IndexInput openInput(String indexFile, IOContext ioContext) throws IOException {
        LOG.debug("open input " + indexFile);
        return new FdsIndexInput(io, indexFile, domain, volume, blobName(indexFile), objectSize);
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

}

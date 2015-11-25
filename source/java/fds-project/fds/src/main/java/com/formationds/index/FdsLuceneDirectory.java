package com.formationds.index;

import com.formationds.hadoop.OwnerGroupInfo;
import com.formationds.nfs.*;
import com.formationds.nfs.deferred.CacheEntry;
import com.formationds.nfs.deferred.EvictingCache;
import com.formationds.util.ServerPortFinder;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.RealAsyncAm;
import org.apache.log4j.Logger;
import org.apache.lucene.store.*;
import org.joda.time.Duration;
import org.junit.Ignore;

import java.io.IOException;
import java.io.OutputStream;
import java.util.Collection;
import java.util.concurrent.TimeUnit;

@Ignore
public class FdsLuceneDirectory extends Directory {
    private static final Logger LOG = Logger.getLogger(FdsLuceneDirectory.class);
    public static final String INDEX_FILE_PREFIX = "index/";
    public static final OwnerGroupInfo OWNER = new OwnerGroupInfo("fds", "fds");
    public static final String SIZE = "SIZE";
    public static final String LUCENE_RESOURCE_NAME = "NAME";
    private String domain;
    private String volume;
    private int objectSize;
    private EvictingCache<String, MemoryLock> locks;
    private TransactionalIo io;


    public FdsLuceneDirectory(IoOps ops, String domain, String volume, int objectSize) {
        init(domain, volume, objectSize);
        io = new TransactionalIo(ops);
    }

    private void init(String domain, String volume, int objectSize) {
        this.domain = domain;
        this.volume = volume;
        this.objectSize = objectSize;
        locks = new EvictingCache<>(
                (key, entry) -> entry.value.invalidate(),
                "Lucene-FDS locks",
                1000000, 1, TimeUnit.HOURS);
        locks.start();
    }

    public FdsLuceneDirectory(String domain, String volume, int objectSize, String amHost, int amPort) throws IOException {
        init(domain, volume, objectSize);
        int serverPort = new ServerPortFinder().findPort("FdsLuceneDirectory AM client", 10000);
        AsyncAm asyncAm = new RealAsyncAm(amHost, amPort, serverPort, Duration.standardSeconds(30));
        asyncAm.start();
        Counters counters = new Counters();
        AmOps amOps = new AmOps(asyncAm, counters);
        DeferredIoOps deferredIo = new DeferredIoOps(amOps, counters);
        deferredIo.start();
        io = new TransactionalIo(deferredIo);
    }

    @Override
    public String[] listAll() throws IOException {
        return io.scan(domain, volume, INDEX_FILE_PREFIX,
                (x, metadata) -> metadata.get().get(LUCENE_RESOURCE_NAME)).toArray(new String[0]);
    }

    @Override
    public void deleteFile(String s) throws IOException {
        io.deleteBlob(domain, volume, blobName(s));
    }

    @Override
    public long fileLength(String s) throws IOException {
        String indexFile = blobName(s);
        return io.mapMetadata(domain, volume, indexFile, (x, metadata) -> {
            if (!metadata.isPresent()) {
                return 0l;
            }

            return Long.parseLong(metadata.get().get(SIZE));
        });
    }

    @Override
    public IndexOutput createOutput(String fileName, IOContext ioContext) throws IOException {
        String blobName = blobName(fileName);
        boolean exists = io.mapMetadata(domain, volume, blobName, (x, metadata) -> metadata.isPresent());
        if (exists) {
            io.deleteBlob(domain, volume, blobName);
        }

        OutputStream out = new ChunkedOutputStream(io, fileName, domain, volume, blobName, objectSize);
        return new OutputStreamIndexOutput(fileName, out, objectSize);
    }

    @Override
    public void sync(Collection<String> collection) throws IOException {
        io.flush();
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
    public Lock obtainLock(String name) throws IOException {
        return locks.lock(name, c -> c.computeIfAbsent(name, k -> new CacheEntry<>(new MemoryLock(), true))).value;
    }

    @Override
    public void close() throws IOException {
        io.flush();
    }

}

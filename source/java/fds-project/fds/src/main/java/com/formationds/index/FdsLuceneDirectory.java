package com.formationds.index;

import com.formationds.hadoop.OwnerGroupInfo;
import com.formationds.nfs.*;
import com.formationds.util.ServerPortFinder;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.RealAsyncAm;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import org.apache.log4j.Logger;
import org.apache.lucene.store.*;
import org.joda.time.Duration;
import org.junit.Ignore;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.OutputStream;
import java.util.*;
import java.util.concurrent.ExecutionException;

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
    private Cache<SimpleKey, MemoryLock> locks;
    private IoOps io;


    public FdsLuceneDirectory(IoOps ops, String domain, String volume, int objectSize) {
        init(domain, volume, objectSize);
        io = ops;
    }

    private void init(String domain, String volume, int objectSize) {
        this.domain = domain;
        this.volume = volume;
        this.objectSize = objectSize;
        locks = CacheBuilder
                .newBuilder()
                .maximumSize(1000000)
                .build();
    }

    public FdsLuceneDirectory(String domain, String volume, int maxObjectSize, String amHost, int amPort) throws IOException {
        init(domain, volume, maxObjectSize);
        int serverPort = new ServerPortFinder().findPort("FdsLuceneDirectory AM client", 10000);
        AsyncAm asyncAm = new RealAsyncAm(amHost, amPort, serverPort, Duration.standardSeconds(30));
        asyncAm.start();
        AmOps amOps = new AmOps(asyncAm);
        DeferredIoOps deferredIo = new DeferredIoOps(amOps, v -> maxObjectSize);
        deferredIo.start();
        io = deferredIo;
    }

    @Override
    public String[] listAll() throws IOException {
        Collection<BlobMetadata> bms = io.scan(domain, volume, INDEX_FILE_PREFIX);
        List<String> result = new ArrayList<>(bms.size());
        for (BlobMetadata bm : bms) {
            result.add(bm.getMetadata().get(LUCENE_RESOURCE_NAME));
        }
        return result.toArray(new String[0]);
    }

    @Override
    public void deleteFile(String s) throws IOException {
        io.deleteBlob(domain, volume, blobName(s));
    }

    @Override
    public long fileLength(String s) throws IOException {
        String indexFile = blobName(s);
        Optional<Map<String, String>> ofm = io.readMetadata(domain, volume, indexFile);
        if (!ofm.isPresent()) {
            return 0;
        } else {
            return Long.parseLong(ofm.get().getOrDefault(SIZE, "0"));
        }
    }

    @Override
    public IndexOutput createOutput(String fileName, IOContext ioContext) throws IOException {
        String blobName = blobName(fileName);
        boolean exists = io.readMetadata(domain, volume, blobName).isPresent();
        if (exists) {
            io.deleteBlob(domain, volume, blobName);
        }

        OutputStream out = new ChunkedOutputStream(io, fileName, domain, volume, blobName, objectSize);
        return new OutputStreamIndexOutput(fileName, out, objectSize);
    }

    @Override
    public void sync(Collection<String> collection) throws IOException {
        io.commitAll();
    }

    @Override
    public void renameFile(String from, String to) throws IOException {
        LOG.debug("Rename file " + from + " " + to);
        String blobName = blobName(from);

        Optional<Map<String, String>> ofm = io.readMetadata(domain, volume, blobName);
        if (!ofm.isPresent()) {
            throw new FileNotFoundException("Volume=" + volume + ", blobName=" + blobName);
        }

        Map<String, String> metadata = new HashMap<>();
        metadata.put(LUCENE_RESOURCE_NAME, to);
        io.writeMetadata(domain, volume, blobName, metadata);
        io.commitMetadata(domain, volume, blobName);
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
        SimpleKey key = new SimpleKey(name);
        try {
            return locks.get(key, () -> new MemoryLock());
        } catch (ExecutionException e) {
            if (e.getCause() instanceof IOException) {
                throw (IOException) e.getCause();
            } else {
                throw new IOException(e);
            }
        }
    }

    @Override
    public void close() throws IOException {
        io.commitAll();
    }

}

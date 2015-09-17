package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.xdi.AsyncAm;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;

public class AmIo implements Io {
    private static final Logger LOG = Logger.getLogger(AmIo.class);
    private Io io;

    public AmIo(AsyncAm am, Counters counters, boolean deferMetadataWrites) {
        DirectAmIo directAmIo = new DirectAmIo(am, counters);
        if (deferMetadataWrites) {
            LOG.info("XDI/NFS connector using deferred metadata write strategy");
            DeferredMetadataWriter deferrer = new DeferredMetadataWriter(directAmIo, am, counters);
            deferrer.start();
            io = new IoCache(deferrer, counters);
        } else {
            io = new IoCache(directAmIo, counters);
        }
    }

    @Override
    public <T> T mapMetadata(String domain, String volumeName, String blobName, MetadataMapper<T> mapper) throws IOException {
        return io.mapMetadata(domain, volumeName, blobName, mapper);
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, MetadataMutator mutator) throws IOException {
        io.mutateMetadata(domain, volume, blobName, mutator);
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, Map<String, String> map, boolean deferrable) throws IOException {
        io.mutateMetadata(domain, volume, blobName, map, deferrable);
    }

    @Override
    public <T> T mapObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMapper<T> objectMapper) throws IOException {
        return io.mapObjectAndMetadata(domain, volume, blobName, objectSize, objectOffset, objectMapper);
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMutator mutator) throws IOException {
        io.mutateObjectAndMetadata(domain, volume, blobName, objectSize, objectOffset, mutator);
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, Map<String, String> metadata) throws IOException {
        io.mutateObjectAndMetadata(domain, volume, blobName, objectSize, objectOffset, byteBuffer, metadata);
    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        io.deleteBlob(domain, volume, blobName);
    }

    @Override
    public <T> List<T> scan(String domain, String volume, String blobNamePrefix, MetadataMapper<T> mapper) throws IOException {
        return io.scan(domain, volume, blobNamePrefix, mapper);
    }
}

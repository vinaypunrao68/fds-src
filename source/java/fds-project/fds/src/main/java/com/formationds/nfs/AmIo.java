package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.xdi.AsyncAm;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;

public class AmIo implements Io {
    private Io io;

    public AmIo(AsyncAm am) {
        io = new IoCache(new DirectAmIo(am));
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
    public void setMetadataOnEmptyBlob(String domain, String volume, String blobName, Map<String, String> map) throws IOException {
        io.setMetadataOnEmptyBlob(domain, volume, blobName, map);
    }

    @Override
    public <T> T mapObject(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMapper<T> objectMapper) throws IOException {
        return io.mapObject(domain, volume, blobName, objectSize, objectOffset, objectMapper);
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

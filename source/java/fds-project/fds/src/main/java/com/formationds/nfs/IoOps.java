package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Collection;
import java.util.Map;
import java.util.Optional;

public interface IoOps {
    public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException;

    public void writeMetadata(String domain, String volumeName, String blobName, Map<String, String> metadata, boolean deferrable) throws IOException;

    public ByteBuffer readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int objectSize) throws IOException;

    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, ByteBuffer byteBuffer, int objectSize, boolean deferrable) throws IOException;

    public void deleteBlob(String domain, String volumeName, String blobName) throws IOException;

    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException;

    public Collection<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException;

    public void flush() throws IOException;
}

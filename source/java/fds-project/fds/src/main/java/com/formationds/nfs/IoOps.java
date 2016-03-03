package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.io.IOException;
import java.util.Collection;
import java.util.Map;
import java.util.Optional;

public interface IoOps {
    public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException;

    public void writeMetadata(String domain, String volumeName, String blobName, Map<String, String> metadata) throws IOException;

    public void commitMetadata(String domain, String volumeName, String blobName) throws IOException;

    public FdsObject readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int maxObjectSize) throws IOException;

    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, FdsObject fdsObject) throws IOException;

    public void commitObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset) throws IOException;

    public void deleteBlob(String domain, String volumeName, String blobName) throws IOException;

    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException;

    public Collection<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException;

    public void onVolumeDeletion(String domain, String volumeName) throws IOException;

    public void commitAll() throws IOException;
}

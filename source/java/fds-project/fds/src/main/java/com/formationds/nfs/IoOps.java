package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.IoConsumer;

import java.io.IOException;
import java.util.Collection;
import java.util.Map;
import java.util.Optional;

public interface IoOps {
    Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException;

    void writeMetadata(String domain, String volumeName, String blobName, Map<String, String> metadata) throws IOException;

    void commitMetadata(String domain, String volumeName, String blobName) throws IOException;

    FdsObject readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int maxObjectSize) throws IOException;

    void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, FdsObject fdsObject) throws IOException;

    void commitObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset) throws IOException;

    void deleteBlob(String domain, String volumeName, String blobName) throws IOException;

    void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException;

    Collection<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException;

    void onVolumeDeletion(String domain, String volumeName) throws IOException;

    void addCommitListener(IoConsumer<MetaKey> listener);

    void commitAll() throws IOException;

    void commitAll(String domain, String volumeName) throws IOException;
}

package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

// TODO: make it domain/volume aware
public class MemoryIoOps implements IoOps {
    private Map<ObjectKey, byte[]> objectCache;
    private Map<MetaKey, Map<String, String>> metadataCache;

    public MemoryIoOps() {
        objectCache = new HashMap<>();
        metadataCache = new HashMap<>();
    }

    @Override
    public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException {
        Map<String, String> map = metadataCache.get(new MetaKey(domain, volumeName, blobName));
        return map == null ? Optional.empty() : Optional.of(map);
    }

    @Override
    public void writeMetadata(String domain, String volumeName, String blobName, Map<String, String> metadata, boolean deferrable) throws IOException {
        metadataCache.put(new MetaKey(domain, volumeName, blobName), metadata);
    }

    @Override
    public ByteBuffer readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int objectSize) throws IOException {
        if (!metadataCache.containsKey(new MetaKey(domain, volumeName, blobName))) {
            throw new FileNotFoundException();
        }

        ObjectKey key = new ObjectKey(domain, volumeName, blobName, objectOffset);
        byte[] bytes = objectCache.get(key);
        if (bytes == null) {
            bytes = new byte[objectSize];
            objectCache.put(key, bytes);
        }
        return ByteBuffer.wrap(bytes);
    }

    @Override
    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, ByteBuffer byteBuffer, int objectSize) throws IOException {
        if (byteBuffer.remaining() != objectSize) {
            throw new IOException("Attempting to write a buffer where size != objectSize");
        }

        byte[] bytes = new byte[objectSize];
        byteBuffer.get(bytes);
        objectCache.put(new ObjectKey(domain, volumeName, blobName, objectOffset), bytes);
    }

    @Override
    public List<Map<String, String>> scan(String domain, String volume, String blobNamePrefix) throws IOException {
        return metadataCache.keySet()
                .stream()
                .filter(k -> k.blobName.startsWith(blobNamePrefix))
                .map(k -> metadataCache.get(k))
                .collect(Collectors.toList());
    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        objectCache.keySet().forEach(ok -> {
            if (ok.blobName.equals(blobName)) {
                objectCache.remove(ok);
            }
        });
        metadataCache.remove(blobName);
    }
}

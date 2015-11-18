package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.apache.log4j.Logger;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.stream.Collectors;

public class MemoryIoOps implements IoOps {
    private static final Logger LOG = Logger.getLogger(MemoryIoOps.class);
    private Map<ObjectKey, byte[]> objectCache;
    private Map<MetaKey, Map<String, String>> metadataCache;

    public MemoryIoOps() {
        objectCache = new HashMap<>();
        metadataCache = new HashMap<>();
    }

    @Override
    public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException {
        Map<String, String> map = metadataCache.get(new MetaKey(domain, volumeName, blobName));
        Optional<Map<String, String>> result = map == null ? Optional.empty() : Optional.of(map);
        LOG.debug("MemoryIO.readMetadata, volume=" + volumeName + ", blobName=" + blobName + ", fieldCount=" + result.orElse(new HashMap<>()).size());
        return result;
    }

    @Override
    public void writeMetadata(String domain, String volumeName, String blobName, Map<String, String> metadata, boolean deferrable) throws IOException {
        metadataCache.put(new MetaKey(domain, volumeName, blobName), metadata);
        LOG.debug("MemoryIO.writeMetadata, volume=" + volumeName + ", blobName=" + blobName + ", fieldCount=" + metadata.size());
    }

    @Override
    public ByteBuffer readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int objectSize) throws IOException {
        if (!metadataCache.containsKey(new MetaKey(domain, volumeName, blobName))) {
            throw new FileNotFoundException();
        }
        ObjectKey key = new ObjectKey(domain, volumeName, blobName, objectOffset);

        if (!objectCache.containsKey(key)) {
            throw new FileNotFoundException();
        }

        byte[] bytes = objectCache.get(key);
        if (bytes == null) {
            bytes = new byte[objectSize];
            objectCache.put(key, bytes);
        }
        LOG.debug("MemoryIO.readCompleteObject, volume=" + volumeName + ", blobName=" + blobName + ", objectOffset = " + objectOffset.getValue() + ", size=" + bytes.length);
        return ByteBuffer.wrap(bytes);
    }

    @Override
    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, ByteBuffer byteBuffer, int objectSize, boolean deferrable) throws IOException {
        if (byteBuffer.remaining() != objectSize) {
            throw new IOException("Attempting to write a buffer where size != objectSize");
        }

        byte[] bytes = new byte[objectSize];
        byteBuffer.get(bytes);
        objectCache.put(new ObjectKey(domain, volumeName, blobName, objectOffset), bytes);
        LOG.debug("MemoryIO.writeObject, volume=" + volumeName + ", blobName=" + blobName + ", objectOffset = " + objectOffset.getValue() + ", size=" + bytes.length);
    }

    @Override
    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException {
        MetaKey oldMetaKey = new MetaKey(domain, volumeName, oldName);
        Map<String, String> map = metadataCache.get(oldMetaKey);
        if (map != null) {
            metadataCache.remove(oldMetaKey);
            metadataCache.put(new MetaKey(domain, volumeName, newName), map);

            HashSet<ObjectKey> objects = new HashSet<>(objectCache.keySet());
            for (ObjectKey oldObjectKey : objects) {
                if (oldObjectKey.domain.equals(domain) && oldObjectKey.volume.equals(volumeName) && oldObjectKey.blobName.equals(oldName)) {
                    byte[] bytes = objectCache.get(oldObjectKey);
                    objectCache.remove(oldObjectKey);
                    objectCache.put(new ObjectKey(domain, volumeName, newName, new ObjectOffset(oldObjectKey.objectOffset)), bytes);
                }
            }
            LOG.debug("MemoryIO.renameBlob, volume=" + volumeName + ", oldName=" + oldName + ", newName=" + newName);
        }
    }

    @Override
    public List<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException {
        List<BlobMetadata> result = metadataCache.keySet()
                .stream()
                .filter(k -> k.blobName.startsWith(blobNamePrefix))
                .map(k -> new BlobMetadata(k.blobName, metadataCache.get(k)))
                .collect(Collectors.toList());
        LOG.debug("MemoryIO.scan, volume=" + volume + ", prefix=" + blobNamePrefix + ", count=" + result.size());
        return result;
    }

    @Override
    public void flush() throws IOException {

    }

    @Override
    public void deleteBlob(String domain, String volume, String blobName) throws IOException {
        Set<ObjectKey> keys = new HashSet<>(objectCache.keySet());
        keys.forEach(ok -> {
            if (ok.blobName.equals(blobName)) {
                objectCache.remove(ok);
            }
        });
        metadataCache.remove(new MetaKey(domain, volume, blobName));
        LOG.debug("MemoryIO.deleteBlob, volume=" + volume + ", blobName=" + blobName);
    }
}

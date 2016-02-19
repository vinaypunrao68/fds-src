package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.util.*;
import java.util.stream.Collectors;

public class MemoryIoOps implements IoOps {
    private static final Logger LOG = Logger.getLogger(MemoryIoOps.class);
    private Map<ObjectKey, FdsObject> objectCache;
    private Map<MetaKey, FdsMetadata> metadataCache;

    public MemoryIoOps() {
        objectCache = new HashMap<>();
        metadataCache = new HashMap<>();
    }

    @Override
    public Optional<FdsMetadata> readMetadata(String domain, String volumeName, String blobName) throws IOException {
        FdsMetadata map = metadataCache.get(new MetaKey(domain, volumeName, blobName));
        Optional<FdsMetadata> result = map == null ? Optional.empty() : Optional.of(map);
        LOG.debug("MemoryIO.readMetadata, volume=" + volumeName + ", blobName=" + blobName + ", fieldCount=" + result.orElse(new FdsMetadata()).fieldCount());
        return result;
    }

    @Override
    public void writeMetadata(String domain, String volumeName, String blobName, FdsMetadata metadata) throws IOException {
        metadataCache.put(new MetaKey(domain, volumeName, blobName), metadata);
        LOG.debug("MemoryIO.writeMetadata, volume=" + volumeName + ", blobName=" + blobName + ", fieldCount=" + metadata.fieldCount());
    }

    @Override
    public void commitMetadata(String domain, String volumeName, String blobName) throws IOException {

    }

    @Override
    public FdsObject readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int maxObjectSize) throws IOException {
        ObjectKey key = new ObjectKey(domain, volumeName, blobName, objectOffset);

        FdsObject o = objectCache.get(key);
        if (o == null) {
            o = FdsObject.allocate(0, maxObjectSize);
            objectCache.put(key, o);
        }
        LOG.debug("MemoryIO.readCompleteObject, volume=" + volumeName + ", blobName=" + blobName + ", objectOffset = " + objectOffset.getValue() + ", limit=" + o.limit() + ", capacity=" + o.maxObjectSize());
        return o;
    }

    @Override
    public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, FdsObject fdsObject) throws IOException {
        objectCache.put(new ObjectKey(domain, volumeName, blobName, objectOffset), fdsObject);
        LOG.debug("MemoryIO.writeObject, volume=" + volumeName + ", blobName=" + blobName + ", objectOffset = " + objectOffset.getValue() + ", size=" + fdsObject.limit());
    }

    @Override
    public void commitObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset) throws IOException {

    }

    @Override
    public void renameBlob(String domain, String volumeName, String oldName, String newName) throws IOException {
        MetaKey oldMetaKey = new MetaKey(domain, volumeName, oldName);
        FdsMetadata map = metadataCache.get(oldMetaKey);
        if (map != null) {
            metadataCache.remove(oldMetaKey);
            metadataCache.put(new MetaKey(domain, volumeName, newName), map);

            HashSet<ObjectKey> objects = new HashSet<>(objectCache.keySet());
            for (ObjectKey oldObjectKey : objects) {
                if (oldObjectKey.domain.equals(domain) && oldObjectKey.volume.equals(volumeName) && oldObjectKey.blobName.equals(oldName)) {
                    FdsObject fdsObject = objectCache.get(oldObjectKey);
                    objectCache.remove(oldObjectKey);
                    objectCache.put(new ObjectKey(domain, volumeName, newName, new ObjectOffset(oldObjectKey.objectOffset)), fdsObject);
                }
            }
            LOG.debug("MemoryIO.renameBlob, volume=" + volumeName + ", oldName=" + oldName + ", newName=" + newName);
        }
    }

    @Override
    public Collection<BlobMetadata> scan(String domain, String volume, String blobNamePrefix) throws IOException {
        List<BlobMetadata> result = metadataCache.keySet()
                .stream()
                .filter(k -> k.blobName.startsWith(blobNamePrefix))
                .map(k -> new BlobMetadata(k.blobName, metadataCache.get(k)))
                .collect(Collectors.toList());
        LOG.debug("MemoryIO.scan, volume=" + volume + ", prefix=" + blobNamePrefix + ", count=" + result.size());
        return result;
    }

    @Override
    public void commitAll() throws IOException {

    }

    @Override
    public void onVolumeDeletion(String domain, String volumeName) throws IOException {
        Iterator<MetaKey> metaKeys = metadataCache.keySet().iterator();
        while (metaKeys.hasNext()) {
            MetaKey next = metaKeys.next();
            if (next.volume.equals(volumeName)) {
                metadataCache.remove(next);
            }
        }

        Iterator<ObjectKey> objectKeys = objectCache.keySet().iterator();
        while (objectKeys.hasNext()) {
            ObjectKey next = objectKeys.next();
            if (next.volume.equals(volumeName)) {
                objectCache.remove(next);
            }
        }
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

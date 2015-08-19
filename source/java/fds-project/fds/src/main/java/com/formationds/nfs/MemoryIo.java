package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

// TODO: make it domain/volume aware
public class MemoryIo implements Io {
    private Map<ObjectKey, byte[]> objectCache;
    private Map<String, Map<String, String>> metadataCache;

    public MemoryIo() {
        objectCache = new HashMap<>();
        metadataCache = new HashMap<>();
    }

    @Override
    public <T> T mapMetadata(String domain, String volumeName, String blobName, MetadataMapper<T> mapper) throws IOException {
        Map<String, String> map = metadataCache.get(blobName);
        if (map == null) {
            return mapper.map(Optional.empty());
        } else {
            return mapper.map(Optional.of(map));
        }
    }

    @Override
    public void mutateMetadata(String domain, String volume, String blobName, MetadataMutator mutator) throws IOException {
        Map<String, String> map = metadataCache.get(blobName);
        Map<String, String> mutated = null;
        if (map == null) {
            mutated = mutator.mutateOrCreate(Optional.empty());
        } else {
            mutated = mutator.mutateOrCreate(Optional.of(map));
        }

        metadataCache.put(blobName, mutated);
    }

    @Override
    public <T> T mapObject(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMapper<T> objectMapper) throws IOException {
        Map<String, String> metadata = metadataCache.get(blobName);
        if (metadata == null) {
            try {
                return objectMapper.map(Optional.empty());
            } catch (Exception e) {
                throw new IOException(e);
            }
        }

        byte[] bytes = objectCache.compute(new ObjectKey(blobName, objectOffset), (k, v) -> {
            if (v == null) {
                v = new byte[objectSize];
            }

            return v;
        });

        try {
            return objectMapper.map(Optional.of(new ObjectView(metadata, ByteBuffer.wrap(bytes))));
        } catch (Exception e) {
            throw new IOException(e);
        }
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ObjectMutator mutator) throws IOException {
        mapObject(domain, volume, blobName, objectSize, objectOffset, (oov) -> {
            ObjectView result = mutator.mutateOrCreate(oov);
            objectCache.put(new ObjectKey(blobName, objectOffset), result.getBuf().array());
            metadataCache.put(blobName, result.getMetadata());
            return null;
        });
    }

    @Override
    public void mutateObjectAndMetadata(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, Map<String, String> metadata) throws IOException {
        objectCache.put(new ObjectKey(blobName, objectOffset), byteBuffer.array());
        metadataCache.put(blobName, metadata);
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


    private class ObjectKey {
        String blobName;
        long objectOffset;

        public ObjectKey(String blobName, ObjectOffset objectOffset) {
            this.blobName = blobName;
            this.objectOffset = objectOffset.getValue();
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            ObjectKey objectKey = (ObjectKey) o;

            if (objectOffset != objectKey.objectOffset) return false;
            if (!blobName.equals(objectKey.blobName)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = blobName.hashCode();
            result = 31 * result + (int) (objectOffset ^ (objectOffset >>> 32));
            return result;
        }
    }
}

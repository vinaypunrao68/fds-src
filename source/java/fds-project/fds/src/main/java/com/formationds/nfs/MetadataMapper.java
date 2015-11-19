package com.formationds.nfs;

import java.io.IOException;
import java.util.Map;
import java.util.Optional;

public interface MetadataMapper<T> {
    public T map(String blobName, Optional<Map<String, String>> metadata) throws IOException;
}

package com.formationds.nfs;

import java.io.IOException;
import java.util.Map;
import java.util.Optional;

interface MetadataMapper<T> {
    public T map(Optional<Map<String, String>> metadata) throws IOException;
}

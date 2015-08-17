package com.formationds.nfs;

import java.io.IOException;
import java.util.Map;
import java.util.Optional;

interface MetadataMutator {
    public Map<String, String> mutateOrCreate(Optional<Map<String, String>> metadata) throws IOException;
}

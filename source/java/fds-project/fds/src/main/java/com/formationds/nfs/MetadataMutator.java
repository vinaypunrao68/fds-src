package com.formationds.nfs;

import java.io.IOException;
import java.util.Map;

interface MetadataMutator {
    public void mutate(Map<String, String> metadata) throws IOException;
}

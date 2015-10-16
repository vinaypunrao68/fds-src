package com.formationds.nfs;

import java.io.IOException;
import java.util.Map;

public interface MetadataMutator<T> {
    public T mutate(Map<String, String> metadata) throws IOException;
}

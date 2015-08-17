package com.formationds.nfs;

import java.io.IOException;
import java.util.Optional;

public interface ObjectMapper<T> {
    public T map(Optional<ObjectView> oov) throws IOException;
}


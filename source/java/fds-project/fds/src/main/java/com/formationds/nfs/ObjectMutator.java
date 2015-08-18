package com.formationds.nfs;

import java.io.IOException;
import java.util.Optional;

public interface ObjectMutator {
    public ObjectView mutateOrCreate(Optional<ObjectView> oov) throws IOException;
}

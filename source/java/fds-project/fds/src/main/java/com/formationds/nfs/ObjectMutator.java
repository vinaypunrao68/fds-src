package com.formationds.nfs;

import java.io.IOException;

public interface ObjectMutator {
    public void mutate(ObjectAndMetadata ov) throws IOException;
}

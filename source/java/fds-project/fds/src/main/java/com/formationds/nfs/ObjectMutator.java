package com.formationds.nfs;

import java.io.IOException;

public interface ObjectMutator {
    public void mutate(ObjectView ov) throws IOException;
}

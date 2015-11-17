package com.formationds.sc;

import java.nio.ByteBuffer;
import java.util.AbstractList;
import java.util.List;

public class Dmt {
    private long version;
    private long[][] lookup;

    public Dmt(long version, long[][] lookup) {
        this.version = version;
        this.lookup = lookup;
    }

    public Dmt(ByteBuffer dmtData) {
        update(dmtData);
    }

    public void update(ByteBuffer dmtData) {
        version = dmtData.getLong();
        int depth = dmtData.getInt();
        int width = dmtData.getInt();

        lookup = new long[width][depth];
        for(int col = 0; col < width; col++) {
            lookup[col] = new long[depth];
            for(int row = 0; row < depth; row++) {
                lookup[col][row] = dmtData.getLong();
            }
        }
    }

    public long getVersion() {
        return version;
    }

    public long[] getDmUuidsForVolumeId(long volumeId) {
        return lookup[(int)(volumeId % lookup.length)];
    }
}

package com.formationds.spike.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;

import java.util.ArrayList;

public class WriteVerifyOperationsWrapper implements NbdServerOperations {
    private NbdServerOperations inner;

    public WriteVerifyOperationsWrapper(NbdServerOperations inner) {
        this.inner = inner;
    }

    @Override
    public boolean exists(String exportName) {
        return inner.exists(exportName);
    }

    @Override
    public long size(String exportName) {
        return inner.size(exportName);
    }

    @Override
    public void read(String exportName, ByteBuf target, long offset, int len) throws Exception {
        inner.read(exportName, target, offset, len);
    }

    @Override
    public void write(String exportName, ByteBuf source, long offset, int len) throws Exception {
        inner.write(exportName, source, offset, len);

        ByteBuf v_buf = Unpooled.buffer(len);
        read(exportName, v_buf, offset, len);
        if(v_buf.compareTo(source) != 0) {
            ArrayList<Integer> differences = new ArrayList<>();
            for (int i = 0; i < len; i++) {
                if (v_buf.getByte(i) != source.getByte(i))
                    differences.add(i);
            }
            throw new Exception("Write verification failed");
        }
    }
}

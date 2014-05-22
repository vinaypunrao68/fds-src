package com.formationds.spike.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;

public class RamOperations implements NbdServerOperations {
    private String name;
    private byte[] bytes;

    public RamOperations(String exportName, int size) {

        name = exportName;
        bytes = new byte[size];
    }

    @Override
    public boolean exists(String exportName) {
        return exportName.equalsIgnoreCase(name);
    }

    @Override
    public long size(String exportName) {
        return bytes.length;
    }

    @Override
    public void read(String exportName, ByteBuf target, long offset, int len) {
        target.writeBytes(bytes, (int)offset, len);
    }

    @Override
    public void write(String exportName, ByteBuf source, long offset, int len) {
        source.readBytes(bytes, (int)offset, len);

    }
}

package com.formationds.spike.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;

public interface NbdServerOperations {
    boolean exists(String exportName);
    long size(String exportName);
    void read(String exportName, ByteBuf target, long offset, int len) throws Exception;
    void write(String exportName, ByteBuf source, long offset, int len) throws Exception;
}

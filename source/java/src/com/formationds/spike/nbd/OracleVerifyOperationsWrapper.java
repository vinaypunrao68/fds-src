package com.formationds.spike.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.spike.nbd.NbdServerOperations;
import com.formationds.spike.nbd.RamOperations;
import com.sun.javafx.image.ByteToBytePixelConverter;
import io.netty.buffer.ByteBuf;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class OracleVerifyOperationsWrapper implements NbdServerOperations {
    private NbdServerOperations innerOperations;
    private NbdServerOperations oracle;

    public OracleVerifyOperationsWrapper(NbdServerOperations inner, NbdServerOperations oracle) {

        innerOperations = inner;
        this.oracle = oracle;
    }

    @Override
    public boolean exists(String exportName) {
        return innerOperations.exists(exportName);
    }

    @Override
    public long size(String exportName) {
        return innerOperations.size(exportName);
    }

    @Override
    public void read(String exportName, ByteBuf target, long offset, int len) throws Exception {
        ByteBuf ramTarget = target.copy();
        oracle.read(exportName, ramTarget, offset, len);

        innerOperations.read(exportName, target, offset, len);
        ArrayList<Integer> differences = new ArrayList<>();
        if(target.compareTo(ramTarget) != 0) {
            for(int i = 0; i < len; i++) {
                if(target.getByte(i) != ramTarget.getByte(i))
                    differences.add(i);
            }

            if(differences.size() > 0)
                throw new Exception("read buffer does not match oracle");
        }
    }

    @Override
    public void write(String exportName, ByteBuf source, long offset, int len) throws Exception {
        ByteBuf ramSource = source.copy();
        oracle.write(exportName, ramSource, offset, len);
        innerOperations.write(exportName, source, offset, len);
    }
}

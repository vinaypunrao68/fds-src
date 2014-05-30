package com.formationds.spike;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.spike.nbd.NbdServerOperations;
import com.formationds.spike.nbd.RamOperations;
import com.sun.javafx.image.ByteToBytePixelConverter;
import io.netty.buffer.ByteBuf;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class ReadWriteVerifierOperations implements NbdServerOperations {
    private NbdServerOperations innerOperations;
    private Map<String, RamOperations> ramOperationsMap;

    public ReadWriteVerifierOperations(NbdServerOperations inner) {

        innerOperations = inner;
        ramOperationsMap = new HashMap<String, RamOperations>();
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
        RamOperations ramOps = getRamOps(exportName);
        ByteBuf ramTarget = target.copy();
        ramOps.read(exportName, ramTarget, offset, len);

        innerOperations.read(exportName, target, offset, len);
        ArrayList<Integer> differences = new ArrayList<>();
        if(target.compareTo(ramTarget) != 0) {
            for(int i = 0; i < len; i++) {
                if(target.getByte(i) != ramTarget.getByte(i))
                    differences.add(i);
            }
        }
    }

    private RamOperations getRamOps(String exportName) {
        if(!ramOperationsMap.containsKey(exportName)) {
            ramOperationsMap.put(exportName, new RamOperations(exportName, (int)size(exportName)));
        }
        return ramOperationsMap.get(exportName);
    }

    @Override
    public void write(String exportName, ByteBuf source, long offset, int len) throws Exception {
        RamOperations ramOps = getRamOps(exportName);
        ByteBuf ramSource = source.copy();
        ramOps.write(exportName, ramSource, offset, len);
        innerOperations.write(exportName, source, offset, len);
    }
}

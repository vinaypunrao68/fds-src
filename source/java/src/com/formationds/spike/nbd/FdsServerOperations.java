package com.formationds.spike.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import io.netty.buffer.ByteBuf;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

public class FdsServerOperations implements NbdServerOperations {
    private static final Logger LOG = Logger.getLogger(FdsServerOperations.class);

    public static final String FDS = "fds";
    private AmService.Iface am;
    private ConfigurationService.Iface config;
    private final Map<String, Integer> volumeObjectSizes;

    public FdsServerOperations(AmService.Iface am, ConfigurationService.Iface config) throws TException {
        this.am = am;
        this.config = config;
        volumeObjectSizes = new HashMap<>();
        config.listVolumes(FDS).stream()
            .forEach(v -> volumeObjectSizes.put(v.getName(), v.getPolicy().getMaxObjectSizeInBytes()));

    }

    @Override
    public boolean exists(String exportName) {
        return volumeObjectSizes.keySet().contains(exportName);
    }

    @Override
    public long size(String exportName) {
        return 10 * 1024 * 1024;
    }

    @Override
    public void read(String exportName, ByteBuf target, final long offset, int len) {
        int objectSize = volumeObjectSizes.get(exportName);
        ObjectOffset objectOffset = new ObjectOffset(offset / objectSize);
        ByteBuffer buf = null;
        try {
            buf = am.getBlob(FDS, exportName, exportName, len, objectOffset);
        } catch (TException e) {
            LOG.error("error reading bytes", e);
        }
        target.setBytes(0, buf.array());
    }

    @Override
    public void write(String exportName, ByteBuf source, long offset, int len) {
        int objectSize = volumeObjectSizes.get(exportName);
        ObjectOffset objectOffset = new ObjectOffset(offset / objectSize);
        try {
            TxDescriptor txId = am.startBlobTx(FDS, exportName, exportName);
            am.updateBlob(FDS, exportName, exportName, txId, ByteBuffer.wrap(source.array()), len, objectOffset, ByteBuffer.allocate(0), true);
            am.commitBlobTx(txId);
        } catch (TException e) {
            LOG.error("error writing bytes", e);
        }
    }
}

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

import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

public class FdsServerOperations implements NbdServerOperations {
    private static final Logger LOG = Logger.getLogger(FdsServerOperations.class);

    public static final String BLOCK_DEV_NAME = "block_dev_0";
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

    private int getMaxObjectSize(String exportName) {
        return 4096;
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
        int objectSize = getMaxObjectSize(exportName);

        try {
            int am_bytes_read = 0;
            while(am_bytes_read < len) {
                long cur = offset + am_bytes_read;
                long o_off = cur / objectSize;
                int i_off = (int)(cur % objectSize);
                int i_len = Math.min(len - am_bytes_read, objectSize - i_off);

                ObjectOffset objectOffset = new ObjectOffset(o_off);
                ByteBuffer buf = am.getBlob(FDS, exportName, BLOCK_DEV_NAME, objectSize, objectOffset);

                target.writeBytes(buf.array(), i_off, i_len);
                am_bytes_read += i_len;
            }
        } catch (TException e) {
            LOG.error("error reading bytes", e);
        }
    }

    @Override
    public void write(String exportName, ByteBuf source, long offset, int len) {
        int objectSize = getMaxObjectSize(exportName);

        try {
            int am_bytes_written = 0;
            TxDescriptor txId = am.startBlobTx(FDS, exportName, exportName);
            ByteBuffer readBuf = ByteBuffer.wrap(source.array());
            while(am_bytes_written < len) {
                long cur = offset + am_bytes_written;
                long o_off = cur / objectSize;
                ObjectOffset objectOffset = new ObjectOffset(o_off);

                am.updateBlob(FDS, exportName, BLOCK_DEV_NAME, txId, readBuf, objectSize, objectOffset, ByteBuffer.allocate(0), true);
                am_bytes_written += objectSize;
            }
            am.commitBlobTx(txId);
        } catch (TException e) {
            LOG.error("error writing bytes", e);
        }
    }
}

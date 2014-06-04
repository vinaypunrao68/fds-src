package com.formationds.spike.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import io.netty.buffer.ByteBuf;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class FdsServerOperations implements NbdServerOperations {
    private static final Logger LOG = Logger.getLogger(FdsServerOperations.class);

    public static final String BLOCK_DEV_NAME = "block_dev_0";
    public static final String FDS = "fds";
    private AmService.Iface am;
    private ConfigurationService.Iface config;
    private final Map<String, Integer> volumeObjectSizes;
    private final Map<String, Long> volumeCapacity;

    public FdsServerOperations(AmService.Iface am, ConfigurationService.Iface config) throws TException {
        this.am = am;
        this.config = config;
        volumeObjectSizes = new ConcurrentHashMap<>();
        volumeCapacity = new ConcurrentHashMap<>();
    }

    private int getMaxObjectSize(String exportName) {
        return volumeObjectSizes.compute(exportName, (k, v) -> {
            if (v == null) {
                try {
                    v = config.statVolume(FDS, exportName).getPolicy().getMaxObjectSizeInBytes();
                } catch (TException e) {
                    LOG.error("Error getting max object size", e);
                }
            }

            return v;
        });
    }

    @Override
    public boolean exists(String exportName) {
        // FIXME: better exception path?
        try {
             List<VolumeDescriptor> volumes = config.listVolumes(FDS);
            return volumes.stream().anyMatch(vd -> vd.getName().equals(exportName));
        } catch(TException exception) {
            return false;
        }

    }

    @Override
    public long size(String exportName) {
        return volumeCapacity.compute(exportName, (k, v) -> {
            if (v == null) {
                try {
                    v = config.statVolume(FDS, exportName).getPolicy().getBlockDeviceSizeInBytes();
                } catch (TException e) {
                    LOG.error("Can't stat volume", e);
                    v = 0l;
                }
            }

            return v;
        });
    }

    @Override
    public synchronized void read(String exportName, ByteBuf target, final long offset, int len) throws Exception {
        int objectSize = getMaxObjectSize(exportName);

        try {
            int am_bytes_read = 0;
            while(am_bytes_read < len) {
                long cur = offset + am_bytes_read;
                long o_off = cur / objectSize;
                int i_off = (int)(cur % objectSize);
                int i_len = Math.min(len - am_bytes_read, objectSize - i_off);

                ObjectOffset objectOffset = new ObjectOffset(o_off);
                ByteBuffer buf = guardedRead(exportName, objectSize, objectOffset);

                target.writeBytes(buf.array(), i_off, i_len);
                am_bytes_read += i_len;
            }
        } catch (TException e) {
            LOG.error("error reading bytes", e);
            throw e;
        }
    }

    @Override
    public synchronized void write(String exportName, ByteBuf source, long offset, int len) throws Exception {
        int objectSize = getMaxObjectSize(exportName);

        try {
            int am_bytes_written = 0;

            while(am_bytes_written < len) {
                long cur = offset + am_bytes_written;
                long o_off = cur / objectSize;
                int i_off = (int)(cur % objectSize);
                int i_len = Math.min(len - am_bytes_written, objectSize - i_off);
                ObjectOffset objectOffset = new ObjectOffset(o_off);

                TxDescriptor txId = am.startBlobTx(FDS, exportName, exportName);
                if(i_off == 0) {
                    ByteBuffer readBuf =  ByteBuffer.allocate(i_len);
                    System.arraycopy(source.array(), am_bytes_written, readBuf.array(), 0, i_len);
                    am.updateBlob(FDS, exportName, BLOCK_DEV_NAME, txId, readBuf, i_len, objectOffset, false);
                    am_bytes_written += i_len;
                } else {
                    ByteBuffer readBuf = guardedRead(exportName, objectSize, objectOffset);
                    System.arraycopy(source.array(), 0, readBuf.array(), i_off, i_len);
                    am.updateBlob(FDS, exportName, BLOCK_DEV_NAME, txId, readBuf, objectSize, objectOffset, false);
                    am_bytes_written += i_len;
                }
                am.commitBlobTx(txId);
            }

            // write verification
            /*ByteBuf v_buf = Unpooled.buffer(len);
            read(exportName, v_buf, offset, len);
            if(v_buf.compareTo(source) != 0) {
                ArrayList<Integer> differences = new ArrayList<>();
                for(int i = 0; i < len; i++) {
                    if(v_buf.getByte(i) != source.getByte(i))
                        differences.add(i);
                }
                LOG.warn("write verify failed");
            } */
        } catch (TException e) {
            LOG.error("error writing bytes", e);
            throw e;
        }
    }

    private ByteBuffer guardedRead(String exportName, int objectSize, ObjectOffset objectOffset) throws TException {
        try {
            return am.getBlob(FDS, exportName, BLOCK_DEV_NAME, objectSize, objectOffset);
        } catch(ApiException e) {
            if(e.getErrorCode() == ErrorCode.MISSING_RESOURCE)
                return ByteBuffer.allocate(objectSize);

            throw e;
        }
    }
}

package com.formationds.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.commons.Fds;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.apis.*;
import com.formationds.protocol.BlobListOrder;
import com.formationds.xdi.RealAsyncAm;
import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.*;

public class FdsServerOperations implements NbdServerOperations {
    private static final Logger LOG = Logger.getLogger(FdsServerOperations.class);

    public static final String BLOCK_DEV_NAME = "block_dev_0";
    public static final String FDS = "fds";
    private static RealAsyncAm asyncAm;
    private ConfigurationService.Iface config;
    private final Executor executor;
    private final Map<String, Integer> volumeObjectSizes;
    private final Map<String, Long> volumeCapacity;

    public FdsServerOperations(ConfigurationService.Iface config, Executor executor) throws Exception {
        asyncAm = new RealAsyncAm(Fds.getFdsHost(), 8899, 9881, 10, TimeUnit.MINUTES);
        this.config = config;
        this.executor = executor;
        volumeObjectSizes = new ConcurrentHashMap<>();
        volumeCapacity = new ConcurrentHashMap<>();
    }

    public FdsServerOperations(ConfigurationService.Iface config) throws Exception {
        this(config, null);
    }

    private Executor getExecutor() {
        if(executor != null)
            return executor;
        return ForkJoinPool.commonPool();
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
            boolean volumeExists = volumes.stream().anyMatch(vd -> vd.getName().equals(exportName));
            if(!volumeExists)
                return false;

            boolean blobExists = asyncAm.volumeContents(FDS, exportName, 1, 0, "", BlobListOrder.UNSPECIFIED, false).get().stream().anyMatch(bd -> bd.getName().equals(BLOCK_DEV_NAME));
            // do an initial write to create the blob in FDS
            if(!blobExists) {
                ByteBuf buf = Unpooled.buffer(4096);
                write(exportName, buf, 0, 4096).join();
            }
            return true;
        } catch(Exception exception) {
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
    public synchronized CompletableFuture<Void> read(String exportName, ByteBuf target, final long offset, int len) throws Exception {
        CompletableFuture<Void> result = new CompletableFuture<>();

        getExecutor().execute(() -> {
            int objectSize = getMaxObjectSize(exportName);

            try {
                int am_bytes_read = 0;
                while (am_bytes_read < len) {
                    long cur = offset + am_bytes_read;
                    long o_off = cur / objectSize;
                    int i_off = (int) (cur % objectSize);
                    int i_len = Math.min(len - am_bytes_read, objectSize - i_off);

                    ObjectOffset objectOffset = new ObjectOffset(o_off);
                    ByteBuffer buf = ByteBuffer.allocateDirect(guardedRead(exportName, objectSize, objectOffset).get());

                    target.writeBytes(buf.array(), i_off + buf.position(), i_len);
                    am_bytes_read += i_len;

                }
                result.complete(null);
            } catch (TException e) {
                LOG.error("error reading bytes", e);
                result.completeExceptionally(e);
            } catch (InterruptedException e) {
                LOG.error("error reading bytes", e);
                Thread.currentThread().interrupt();
            } catch (ExecutionException e) {
                result.completeExceptionally(e.getCause());
            }
        });
        return result;
    }

    @Override
    public synchronized CompletableFuture<Void> write(String exportName, ByteBuf source, long offset, int len) throws Exception {
        CompletableFuture<Void> result = new CompletableFuture<>();

        getExecutor().execute(() -> {
            int objectSize = getMaxObjectSize(exportName);

            TxDescriptor txId = null;
            try {
                int am_bytes_written = 0;

                while (am_bytes_written < len) {
                    long cur = offset + am_bytes_written;
                    long o_off = cur / objectSize;
                    int i_off = (int) (cur % objectSize);
                    int i_len = Math.min(len - am_bytes_written, objectSize - i_off);
                    ObjectOffset objectOffset = new ObjectOffset(o_off);

                    ByteBuffer readBuf = null;
                    if (i_off != 0 || i_len != objectSize)
                        readBuf = guardedRead(exportName, objectSize, objectOffset);
                    else
                        readBuf = ByteBuffer.allocate(objectSize);
                    System.arraycopy(source.array(), am_bytes_written, readBuf.array(), i_off + readBuf.position(), i_len);

                    if(am_bytes_written == 0) {
                        if(len == i_len) {
                            asyncAm.updateBlobOnce(FDS, exportName, BLOCK_DEV_NAME, 0, readBuf, objectSize, objectOffset, Collections.<String, String>emptyMap());
                            break;
                        } else {
                            txId = asyncAm.startBlobTx(FDS, exportName, BLOCK_DEV_NAME, 0).get();
                        }
                    }

                    asyncAm.updateBlob(FDS, exportName, BLOCK_DEV_NAME, txId, readBuf, objectSize, objectOffset,false).get();
                    am_bytes_written += i_len;
                }

                if(txId != null)
                    asyncAm.commitBlobTx(FDS, exportName, BLOCK_DEV_NAME, txId).get();
                result.complete(null);
            } catch (TException e) {
                LOG.error("error writing bytes", e);
                if(txId != null) {
                    try {
                        asyncAm.abortBlobTx(FDS, exportName, BLOCK_DEV_NAME, txId).get();
                    } catch (InterruptedException e1) {
                        Thread.currentThread().interrupt();
                    } catch (ExecutionException e1) {
                        result.completeExceptionally(e.getCause());
                    }
                }
                result.completeExceptionally(e);
            } catch (InterruptedException e) {
                LOG.error("error writing bytes", e);
                Thread.currentThread().interrupt();
            } catch (ExecutionException e) {
                result.completeExceptionally(e.getCause());
            }
        });
        return result;
    }

    @Override
    public CompletableFuture<Void> flush(String exportName) {
        // TODO: IMPLEMENT ME
        return CompletableFuture.completedFuture(null);
    }

    private ByteBuffer guardedRead(String exportName, int objectSize, ObjectOffset objectOffset) throws TException, ExecutionException, InterruptedException {
        return asyncAm.getBlob(FDS, exportName, BLOCK_DEV_NAME, objectSize, objectOffset).get();
    }
}

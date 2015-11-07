package com.formationds.spike.iscsi;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.FdsObjectFrame;
import org.jscsi.target.storage.IStorageModule;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CompletableFuture;

public class FdsStorageModule implements IStorageModule {

    private static final String ISCSI_BLOB_NAME = "block_dev_0";
    private final String fdsDomain;
    private VolumeDescriptor descriptor;
    private AsyncAm am;

    public FdsStorageModule(AsyncAm am, String fdsDomain, VolumeDescriptor descriptor) {
        this.fdsDomain = fdsDomain;
        this.descriptor = descriptor;
        this.am = am;
    }

    @Override
    public int checkBounds(long logicalBlockAddress, int transferLengthInBlocks) {
        long sz = getSizeInBlocks();
        if(sz <= logicalBlockAddress)
            return 1;
        if(sz <= logicalBlockAddress + transferLengthInBlocks)
            return 2;
        return 0;
    }

    @Override
    public long getSizeInBlocks() {
        return descriptor.getPolicy().getBlockDeviceSizeInBytes() / VIRTUAL_BLOCK_SIZE;
    }

    @Override
    public void read(byte[] bytes, long storageIndex) throws IOException {
        try {
            List<CompletableFuture<Void>> lst = new ArrayList<>();
            int bytesOffset = 0;
            for (FdsObjectFrame frame : FdsObjectFrame.frames(storageIndex, bytes.length, getObjectSize())) {
                lst.add(readFrame(frame, bytes, bytesOffset));
                bytesOffset += frame.internalLength;
            }

            for (CompletableFuture<Void> cf : lst) {
                cf.get();
            }
        } catch(Exception ex) {
            throw new IOException(ex);
        }
    }

    @Override
    public void write(byte[] bytes, long storageIndex) throws IOException {
        try {
            List<CompletableFuture<Void>> lst = new ArrayList<>();
            int writeOffset = 0;
            for (FdsObjectFrame frame : FdsObjectFrame.frames(storageIndex, bytes.length, getObjectSize())) {
                lst.add(writeFrame(frame, bytes, writeOffset));
                writeOffset += frame.internalLength;
            }

            for(CompletableFuture<Void> cf : lst) {
                cf.get();
            }
        } catch (Exception ex) {
            throw new IOException(ex);
        }
    }

    private CompletableFuture<Void> writeFrame(FdsObjectFrame frame, byte[] bytes, int writeOffset) {
        if(frame.internalLength == getObjectSize()) {
            // full object
            ByteBuffer buf = ByteBuffer.wrap(bytes, writeOffset, frame.internalLength);
            return am.updateBlobOnce(fdsDomain, descriptor.getName(), ISCSI_BLOB_NAME, 0, buf, buf.remaining(), new ObjectOffset(frame.objectOffset), Collections.emptyMap());
        } else {
            // read modify write
            byte[] obj = new byte[getObjectSize()];
            return readFrame(frame.entireObject(), obj, 0).thenCompose(_null -> {
                System.arraycopy(bytes, writeOffset, obj, frame.internalOffset, frame.internalLength);
                return writeFrame(frame.entireObject(), obj, 0);
            });
        }
    }

    private CompletableFuture<Void> readFrame(FdsObjectFrame frame, byte[] bytes, int arrayOffset) {
        CompletableFuture<Void> result = new CompletableFuture<>();
        am.getBlob(fdsDomain, descriptor.getName(), ISCSI_BLOB_NAME, frame.internalLength + frame.internalOffset, new ObjectOffset(frame.objectOffset))
                .whenComplete((buf, ex) -> {
                    FdsObjectFrame fr = frame;
                    byte[] b = bytes;
                    int os = arrayOffset;

                    try {
                        if (ex == null) {
                            ByteBuffer slice = buf.slice();
                            int read = 0;

                            if(slice.remaining() > frame.internalOffset) {
                                slice.position(frame.internalOffset);
                                int readLength = Math.min(frame.internalLength, buf.remaining());
                                read += readLength;
                                slice.get(bytes, arrayOffset, readLength);
                            }

                            Arrays.fill(bytes, arrayOffset + read, arrayOffset + frame.internalLength, (byte)0);
                            result.complete(null);

                        } else {
                            if(ex instanceof ApiException && ((ApiException) ex).getErrorCode() == ErrorCode.MISSING_RESOURCE) {
                                Arrays.fill(bytes, arrayOffset, frame.internalLength, (byte)0);
                                result.complete(null);
                            }
                        }
                    } catch (Exception x) {
                        result.completeExceptionally(x);
                    }
                });

        return result;
    }


    @Override
    public void close() throws IOException {

    }

    private int getObjectSize() {
        return descriptor.getPolicy().getMaxObjectSizeInBytes();
    }
}

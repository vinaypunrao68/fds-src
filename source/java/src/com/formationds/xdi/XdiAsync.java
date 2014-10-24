/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.xdi;

import com.formationds.apis.AmService;
import com.formationds.apis.BlobDescriptor;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.util.async.AsyncRequestStatistics;
import com.formationds.util.async.AsyncResourcePool;

import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public interface XdiAsync {

    /**
     *
     */
    interface TransactionHandle {
        public CompletableFuture<Void> commit();
        public CompletableFuture<Void> abort();
        public CompletableFuture<Void> updateMetadata(Map<String, String> meta);
        public CompletableFuture<Void> update(ByteBuffer buffer, long objectOffset, int length);
    }

    /**
     * Holder for a message digest result to a PUT blob request
     */
    class PutResult {
        public final byte[] digest;

        public PutResult(byte[] md) {
            digest = md;
        }
    }

    class BlobInfo {
        public final String domain;
        public final String volume;
        public final String blob;
        public final BlobDescriptor blobDescriptor;
        public final VolumeDescriptor volumeDescriptor;
        public final ByteBuffer object0;

        public BlobInfo(String domain, String volume, String blob, BlobDescriptor bd, VolumeDescriptor vd, ByteBuffer object0) {
            this.domain = domain;
            this.volume = volume;
            this.blob = blob;
            blobDescriptor = bd;
            volumeDescriptor = vd;
            this.object0 = object0;
        }
    }

    // TODO: factor this stuff into somewhere more accessible
    interface FunctionWithExceptions<TIn, TOut> {
        public TOut apply(TIn input) throws Exception;
    }

    XdiAsync withStats(AsyncRequestStatistics statistics);

    CompletableFuture<BlobInfo> getBlobInfo(String domain, String volume, String blob);

    CompletableFuture<PutResult> putBlobFromStream(String domain, String volume, String blob, Map<String, String> metadata, InputStream stream);

    CompletableFuture<Void> getBlobToStream(BlobInfo blob, OutputStream str);

    CompletableFuture<Void> getBlobToConsumer(BlobInfo blobInfo, Function<ByteBuffer, CompletableFuture<Void>> processor);

    CompletableFuture<PutResult> putBlobFromSupplier(String domain, String volume, String blob, Map<String, String> metadata, Function<ByteBuffer, CompletableFuture<Void>> reader);

    <T> CompletableFuture<T> amUseVolume(String volume, AsyncResourcePool.BindWithException<XdiClientConnection<AmService.AsyncIface>, CompletableFuture<T>> operation);

    <T> CompletableFuture<T> csUseVolume(String volume, AsyncResourcePool.BindWithException<XdiClientConnection<ConfigurationService.AsyncIface>, CompletableFuture<T>> operation);

    CompletableFuture<VolumeDescriptor> statVolume(String domain, String volume);

    CompletableFuture<BlobDescriptor> statBlob(String domain, String volume, String blob);

    CompletableFuture<ByteBuffer> getBlob(String domain, String volume, String blob, long offset, int length);

    CompletableFuture<Void> updateBlobOnce(String domain, String volume, String blob, int blobMode, ByteBuffer data, int length, long offset, Map<String, String> metadata);

    CompletableFuture<XdiAsyncImpl.TransactionHandle> createTx(String domain, String volume, String blob, int mode);

}

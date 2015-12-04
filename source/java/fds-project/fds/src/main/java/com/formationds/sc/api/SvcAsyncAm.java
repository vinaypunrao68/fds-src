package com.formationds.sc.api;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.protocol.*;
import com.formationds.protocol.dm.*;
import com.formationds.protocol.dm.types.FDSP_BlobObjectInfo;
import com.formationds.protocol.dm.types.FDSP_MetaDataPair;
import com.formationds.protocol.sm.GetObjectMsg;
import com.formationds.protocol.sm.PutObjectMsg;
import com.formationds.protocol.sm.PutObjectRspMsg;
import com.formationds.protocol.svc.types.FDSP_VolumeDescType;
import com.formationds.protocol.svc.types.FDS_ObjectIdType;
import com.formationds.sc.*;
import com.formationds.util.DigestUtil;
import com.formationds.util.ExceptionMap;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.BlobWithMetadata;
import com.formationds.xdi.VolumeContents;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BiFunction;
import java.util.stream.Collectors;

public class SvcAsyncAm implements AsyncAm {
    private final SvcState svc;
    private final FdsChannels fdsChannels;
    private final long BLOB_VERSION_PLACEHOLDER = 0;  // blob version is not relevant

    public SvcAsyncAm(SvcState svc) {
        this.svc = svc;
        this.fdsChannels = new FdsChannels(svc);
    }

    @Override
    public void start() {
        // assume it's been started already
    }

    @Override
    public CompletableFuture<Void> attachVolume(String domainName, String volumeName) throws TException {
        // return am.attachVolume(domainName, volumeName);
        FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
        return CompletableFutureUtility.voidFutureOf(svc.getVolumeStateSequencer().useVolume(volumeDescriptor.getVolUUID()));
    }

    @Override
    public CompletableFuture<VolumeContents> volumeContents(String domainName, String volumeName, int count, long offset, String pattern, PatternSemantics patternSemantics, String delimiter, BlobListOrder order, boolean descending) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            GetBucketMsg getBucketReq = new GetBucketMsg(volumeDescriptor.getVolUUID(), offset, count, pattern, order, descending, patternSemantics, delimiter);
            CompletableFuture<GetBucketRspMsg> response = fdsChannels.dmRead(volumeDescriptor.getVolUUID(), dm -> dm.getBucket(getBucketReq));
            return response.thenApply(resp ->
                            new VolumeContents(resp.getBlob_descr_list(), resp.getSkipped_prefixes())
            );
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }


    @Override
    public CompletableFuture<BlobDescriptor> statBlob(String domainName, String volumeName, String blobName) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            CompletableFuture<GetBlobMetaDataMsg> blobMeta = fdsChannels.dmRead(volumeDescriptor.getVolUUID(), dm -> dm.getBlobMetaData(volumeDescriptor.getVolUUID(), blobName));
            CompletableFuture<BlobDescriptor> result = blobMeta.thenApply(m -> new BlobDescriptor(m.getBlob_name(), m.getByteCount(), buildMetadataMap(m.getMetaDataList())));

            return statBlobExceptionMap.applyOnFail(result);

        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    private static final ExceptionMap statBlobExceptionMap = new ExceptionMap()
            .map(CompletionException.class, cex -> cex.getCause())
            .mapWhen(SvcException.class, svcEx -> (svcEx.getFdsError() == FdsError.ERR_CAT_ENTRY_NOT_FOUND), svcEx -> new ApiException("blob not found", ErrorCode.MISSING_RESOURCE));

    @Override
    public CompletableFuture<BlobDescriptor> renameBlob(String domainName, String volumeName, String sourceBlobName, String destinationBlobName) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);

            AtomicReference<BlobDescriptor> bd = new AtomicReference<>(null);

            return sequencedDmWrite(volumeDescriptor, (dm, seq) -> {
                long srcTxId = svc.allocateNewTxId();
                long dstTxId = svc.allocateNewTxId();
                RenameBlobMsg renameBlobMsg = new RenameBlobMsg(volumeDescriptor.getVolUUID(), sourceBlobName, destinationBlobName, dm.getDmtVersion(), srcTxId, dstTxId, seq.getSequenceId());

                // FIXME: there's probably a cleaner way to extract the blob descriptor from here
                return dm.renameBlob(renameBlobMsg).thenAccept(result -> {
                    if(bd.get() == null) {
                        BlobDescriptor descriptor = new BlobDescriptor(destinationBlobName, result.getByteCount(), buildMetadataMap(result.getMetaDataList()));
                        bd.accumulateAndGet(descriptor, (current, me) -> current == null ? me : current);
                    }
                });
            }).thenApply(_null -> bd.get());
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<BlobWithMetadata> getBlobWithMeta(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            CompletableFuture<QueryCatalogMsg> catalogFuture =
                    fdsChannels.dmRead(volumeDescriptor.getVolUUID(),
                            dm -> dm.queryCatalog(volumeDescriptor.getVolUUID(), blobName, volumeDescriptor.getMaxObjSizeInBytes(), offset.getValue() * volumeDescriptor.maxObjSizeInBytes, 1));

            return catalogFuture.thenCompose(catalog -> {
                FDSP_BlobObjectInfo object = catalog.obj_list.get(0);
                CompletionStage<ByteBuffer> buffer = getObject(volumeDescriptor.volUUID, object.data_obj_id);
                return buffer.thenApply(buf -> new BlobWithMetadata(buf, new BlobDescriptor(catalog.getBlob_name(), catalog.getByteCount(), buildMetadataMap(catalog.getMeta_list()))));
            });
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<TxDescriptor> startBlobTx(String domainName, String volumeName, String blobName, int blobMode) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            long txId = svc.allocateNewTxId();

            return fdsChannels.dmWrite(volumeDescriptor.getVolUUID(), dm -> {
                StartBlobTxMsg startBlobTxMsg = new StartBlobTxMsg(volumeDescriptor.getVolUUID(), blobName, BLOB_VERSION_PLACEHOLDER, blobMode, txId, dm.getDmtVersion());
                return CompletableFutureUtility.voidFutureOf(dm.startBlobTx(startBlobTxMsg));
            }).thenApply(_null -> new TxDescriptor(txId))
                    .whenComplete(CompletableFutureUtility.trace("txBegin"));
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<Void> commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);

            return sequencedDmWrite(volumeDescriptor, (dm, seq) -> {
                        CommitBlobTxMsg commitBlobTxMsg = new CommitBlobTxMsg(volumeDescriptor.getVolUUID(), blobName, BLOB_VERSION_PLACEHOLDER, txDescriptor.txId, dm.getDmtVersion(), seq.getSequenceId());
                        return dm.commitBlobTx(commitBlobTxMsg);
            });
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<Void> abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            return sequencedDmWrite(volumeDescriptor, (dm, seq) -> {
                        AbortBlobTxMsg abortBlobTxMsg = new AbortBlobTxMsg(volumeDescriptor.getVolUUID(), blobName, BLOB_VERSION_PLACEHOLDER, txDescriptor.txId);
                        return dm.abortBlobTx(abortBlobTxMsg);
                    });
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<ByteBuffer> getBlob(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            CompletableFuture<QueryCatalogMsg> catalogFuture =
                    fdsChannels.dmRead(volumeDescriptor.getVolUUID(), dm -> dm.queryCatalog(volumeDescriptor.getVolUUID(), blobName, volumeDescriptor.getMaxObjSizeInBytes(), offset.getValue(), 1));

            return catalogFuture.thenCompose(catalog -> {
                FDSP_BlobObjectInfo object = catalog.obj_list.get(0);
                return getObject(volumeDescriptor.volUUID, object.data_obj_id);
            });
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<Void> updateMetadata(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor, Map<String, String> metadata) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            List<FDSP_MetaDataPair> metadataPairs = buildMetadataPairList(metadata);

            return sequencedDmWrite(volumeDescriptor, (dm, seq) -> {
                SetBlobMetaDataMsg setBlobMetaDataMsg = new SetBlobMetaDataMsg(volumeDescriptor.getVolUUID(), blobName, BLOB_VERSION_PLACEHOLDER, metadataPairs, txDescriptor.txId);
                return dm.setBlobMetaData(setBlobMetaDataMsg);
            });
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<Void> updateBlob(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            checkPutParameters(volumeDescriptor, bytes, length);

            CompletableFuture<List<FDSP_BlobObjectInfo>> smPut = putObject(volumeDescriptor.getVolUUID(), bytes)
                    .thenApply(objId -> Collections.singletonList(new FDSP_BlobObjectInfo(objectOffset.getValue() * volumeDescriptor.maxObjSizeInBytes, objId, length)));

            return sequencedDmWrite(volumeDescriptor, (dm, seq) ->
                smPut.thenCompose(objList -> {
                    UpdateCatalogMsg updateCatalogMsg = new UpdateCatalogMsg(volumeDescriptor.getVolUUID(), blobName, BLOB_VERSION_PLACEHOLDER, txDescriptor.txId, objList);
                    return dm.updateCatalog(updateCatalogMsg);
                }));
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<Void> updateBlobOnce(String domainName, String volumeName, String blobName, int blobMode, ByteBuffer bytes, int length, ObjectOffset offset, Map<String, String> metadata) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            checkPutParameters(volumeDescriptor, bytes, length);
            List<FDSP_MetaDataPair> metadataPairs = buildMetadataPairList(metadata);
            CompletableFuture<List<FDSP_BlobObjectInfo>> smPut = putObject(volumeDescriptor.getVolUUID(), bytes)
                    .thenApply(objId -> Collections.singletonList(new FDSP_BlobObjectInfo(offset.getValue() * volumeDescriptor.maxObjSizeInBytes, objId, length)));

            long txId = svc.allocateNewTxId();
            return sequencedDmWrite(volumeDescriptor, (dm, seq) ->
                    smPut.thenCompose(lst -> {
                        UpdateCatalogOnceMsg updateCatalogOnceMsg =
                                new UpdateCatalogOnceMsg(volumeDescriptor.getVolUUID(), blobName, BLOB_VERSION_PLACEHOLDER, blobMode, dm.getDmtVersion(), txId, lst, metadataPairs, seq.getSequenceId());
                        return dm.updateCatalogOnce(updateCatalogOnceMsg);
                    }));
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<Void> deleteBlob(String domainName, String volumeName, String blobName) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);

            return startBlobTx(domainName, volumeName, blobName, 0)
                    .thenCompose(txId ->
                        sequencedDmWrite(volumeDescriptor, (dm, seq) -> {
                            DeleteBlobMsg deleteMsg = new DeleteBlobMsg(txId.txId, volumeDescriptor.getVolUUID(), blobName, BLOB_VERSION_PLACEHOLDER);
                            return dm.deleteBlob(deleteMsg);
                        }).thenCompose(_null -> commitBlobTx(domainName, volumeName, blobName, txId))
                            .whenComplete((r, ex) -> { if(ex != null) abortBlobTx(domainName, volumeName, blobName, txId); })
                    );
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<VolumeStatus> volumeStatus(String domainName, String volumeName) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            return fdsChannels.dmRead(volumeDescriptor.getVolUUID(), dm -> dm.statVolume(volumeDescriptor.getVolUUID()))
                    .thenApply(s -> new VolumeStatus(s.getVolumeStatus().getBlobCount(), s.getVolumeStatus().getSize()));
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<Void> setVolumeMetadata(String domainName, String volumeName, Map<String, String> metadata) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            List<FDSP_MetaDataPair> pairs = buildMetadataPairList(metadata);

            return sequencedDmWrite(volumeDescriptor, (dm, seq) -> {
                SetVolumeMetadataMsg msg = new SetVolumeMetadataMsg(volumeDescriptor.getVolUUID(), pairs, seq.getSequenceId());
                return dm.setVolumeMetadata(msg);
            });
        } catch(Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    @Override
    public CompletableFuture<Map<String, String>> getVolumeMetadata(String domainName, String volumeName) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            CompletableFuture<GetVolumeMetadataMsgRsp> volumeMeta = fdsChannels.dmRead(volumeDescriptor.getVolUUID(), dm -> dm.getVolumeMetadata(volumeDescriptor.getVolUUID()));
            return volumeMeta.thenApply(m -> buildMetadataMap(m.getMetadataList()));
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    private Map<String, String> buildMetadataMap(List<FDSP_MetaDataPair> lst) {
        return lst.stream().collect(Collectors.toMap(v -> v.getKey(), v -> v.getValue()));
    }

    private void checkPutParameters(FDSP_VolumeDescType volumeDescriptor, ByteBuffer bytes, long specifiedLength) throws ApiException {
        if(specifiedLength > volumeDescriptor.maxObjSizeInBytes)
            throw new ApiException("Requested update length exceeds volume object size", ErrorCode.BAD_REQUEST);
        if(bytes.remaining() != specifiedLength)
            throw new ApiException("Remaining bytes in update request buffer does not match requested length", ErrorCode.BAD_REQUEST);
    }

    // TODO: add object cache
    private CompletableFuture<ByteBuffer> getObject(long volumeId, FDS_ObjectIdType data_obj_id) {
        return fdsChannels.smRead(data_obj_id.getDigest(), sm -> sm.getObject(new GetObjectMsg(volumeId, data_obj_id)))
                .thenApply(getObjectResp -> getObjectResp.data_obj);
    }

    private CompletableFuture<FDS_ObjectIdType> putObject(long volumeId, ByteBuffer bytes) {
        FDS_ObjectIdType objId = objectId(bytes);
        return fdsChannels.smWrite(objId.getDigest(), sm -> {
            CompletableFuture<PutObjectRspMsg> rspFuture = sm.putObject(new PutObjectMsg(volumeId, objId, false, bytes.remaining(), bytes));
            return CompletableFutureUtility.voidFutureOf(rspFuture);
        }).thenApply(_null -> objId);
    }

    private FDS_ObjectIdType objectId(ByteBuffer bytes) {
        MessageDigest sha1 = DigestUtil.newSha1();
        sha1.update(bytes.duplicate());
        return new FDS_ObjectIdType(ByteBuffer.wrap(sha1.digest()));
    }

    private <T> CompletableFuture<Void> sequencedDmWrite(FDSP_VolumeDescType volumeDescriptor, BiFunction<DmChannel, VolumeStateSequencer.SequenceData, CompletableFuture<T>> operation) {
        CompletableFuture<VolumeStateSequencer.SequenceData> sequenceData = svc.getVolumeStateSequencer().useVolume(volumeDescriptor.getVolUUID());
        return fdsChannels.dmWrite(volumeDescriptor.getVolUUID(), dm ->
                sequenceData.thenCompose(seq ->
                        CompletableFutureUtility.voidFutureOf(operation.apply(dm, seq))));

    }

    private List<FDSP_MetaDataPair> buildMetadataPairList(Map<String, String> metadata) {
        return metadata.entrySet().stream()
                .map(e -> new FDSP_MetaDataPair(e.getKey(), e.getValue()))
                .collect(Collectors.toList());
    }

    private FDSP_VolumeDescType getVolumeDescriptor(String volumeName) throws TException {
        Optional<FDSP_VolumeDescType> volumeId = svc.getVolumeDescriptor(volumeName);
        if(!volumeId.isPresent())
            throw new ApiException("could not find volume with name [" + volumeName + "]", ErrorCode.MISSING_RESOURCE);

        return volumeId.get();
    }
}

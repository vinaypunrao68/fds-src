package com.formationds.sc.api;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.protocol.*;
import com.formationds.protocol.dm.*;
import com.formationds.protocol.dm.types.FDSP_BlobObjectInfo;
import com.formationds.protocol.dm.types.FDSP_MetaDataPair;
import com.formationds.protocol.sm.GetObjectMsg;
import com.formationds.protocol.sm.GetObjectResp;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;
import com.formationds.protocol.svc.types.FDSP_VolumeDescType;
import com.formationds.protocol.svc.types.FDS_ObjectIdType;
import com.formationds.sc.*;
import com.formationds.util.ExceptionMap;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.BlobWithMetadata;
import com.formationds.xdi.VolumeContents;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;
import java.util.concurrent.CompletionStage;
import java.util.stream.Collectors;

public class SvcAsyncAm implements AsyncAm {
    private final AsyncAm am;
    private final SvcState svc;
    private final FdsChannels fdsChannels;

    public SvcAsyncAm(AsyncAm am, SvcState svc) {
        this.am = am;
        this.svc = svc;
        this.fdsChannels = new FdsChannels(svc);

    }

    @Override
    public void start() {
        // assume it's been started already
    }

    @Override
    public CompletableFuture<Void> attachVolume(String domainName, String volumeName) throws TException {
        return am.attachVolume(domainName, volumeName);
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
        } catch(Exception ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }


    @Override
    public CompletableFuture<BlobDescriptor> statBlob(String domainName, String volumeName, String blobName) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            CompletableFuture<GetBlobMetaDataMsg> blobMeta = fdsChannels.dmRead(volumeDescriptor.getVolUUID(), dm -> dm.getBlobMetaData(volumeDescriptor.getVolUUID(), blobName));
            CompletableFuture<BlobDescriptor> result = blobMeta.thenApply(m -> new BlobDescriptor(m.getBlob_name(), m.getByteCount(), buildMetadataMap(m.getMetaDataList())));

            return statBlobExceptionMap.applyOnFail(result);

        } catch (Exception ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    private static final ExceptionMap statBlobExceptionMap = new ExceptionMap()
            .map(CompletionException.class, cex -> cex.getCause())
            .mapWhen(SvcException.class, svcEx -> (svcEx.getFdsError() == FdsError.ERR_CAT_ENTRY_NOT_FOUND), svcEx -> new ApiException("blob not found", ErrorCode.MISSING_RESOURCE));

    private Map<String, String> buildMetadataMap(List<FDSP_MetaDataPair> lst) {
        return lst.stream().collect(Collectors.toMap(v -> v.getKey(), v -> v.getValue()));
    }

    @Override
    public CompletableFuture<BlobDescriptor> renameBlob(String domainName, String volumeName, String sourceBlobName, String destinationBlobName) {
        return am.renameBlob(domainName, volumeName, sourceBlobName, destinationBlobName);
    }

    @Override
    public CompletableFuture<BlobWithMetadata> getBlobWithMeta(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            CompletableFuture<QueryCatalogMsg> catalogFuture =
                    fdsChannels.dmRead(volumeDescriptor.getVolUUID(),
                            dm -> dm.queryCatalog(volumeDescriptor.getVolUUID(), blobName, volumeDescriptor.getMaxObjSizeInBytes(), offset.getValue(), 1));

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
        return am.startBlobTx(domainName, volumeName, blobName, blobMode);
    }

    @Override
    public CompletableFuture<Void> commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return am.commitBlobTx(domainName, volumeName, blobName, txDescriptor);
    }

    @Override
    public CompletableFuture<Void> abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return am.abortBlobTx(domainName, volumeName, blobName, txDescriptor);
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
        return am.updateMetadata(domainName, volumeName, blobName, txDescriptor, metadata);
    }

    @Override
    public CompletableFuture<Void> updateBlob(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) {
        return am.updateBlob(domainName, volumeName, blobName, txDescriptor, bytes, length, objectOffset, isLast);
    }

    @Override
    public CompletableFuture<Void> updateBlobOnce(String domainName, String volumeName, String blobName, int blobMode, ByteBuffer bytes, int length, ObjectOffset offset, Map<String, String> metadata) {
        return am.updateBlobOnce(domainName, volumeName, blobName, blobMode, bytes, length, offset, metadata);
    }

    @Override
    public CompletableFuture<Void> deleteBlob(String domainName, String volumeName, String blobName) {
        return am.deleteBlob(domainName, volumeName, blobName);
    }

    @Override
    public CompletableFuture<VolumeStatus> volumeStatus(String domainName, String volumeName) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            return fdsChannels.dmRead(volumeDescriptor.getVolUUID(), dm -> dm.statVolume(volumeDescriptor.getVolUUID()))
                    .thenApply(s -> new VolumeStatus(s.getVolumeStatus().getBlobCount(), s.getVolumeStatus().getSize()));
        } catch (TException ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    @Override
    public CompletableFuture<Void> setVolumeMetadata(String domainName, String volumeName, Map<String, String> metadata) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            fdsChannels.dmWrite(volumeDescriptor.getVolUUID(), )
            return am.setVolumeMetadata(domainName, volumeName, metadata);
        } catch (TException ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    @Override
    public CompletableFuture<Map<String, String>> getVolumeMetadata(String domainName, String volumeName) {
        try {
            FDSP_VolumeDescType volumeDescriptor = getVolumeDescriptor(volumeName);
            CompletableFuture<GetVolumeMetadataMsgRsp> volumeMeta = fdsChannels.dmRead(volumeDescriptor.getVolUUID(), dm -> dm.getVolumeMetadata(volumeDescriptor.getVolUUID()));
            return volumeMeta.thenApply(m -> buildMetadataMap(m.getMetadataList()));
        } catch (Exception ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    // TODO: add object cache
    private CompletionStage<ByteBuffer> getObject(long volumeId, FDS_ObjectIdType data_obj_id) {
        return fdsChannels.smRead(data_obj_id.getDigest(), sm -> sm.getObject(new GetObjectMsg(volumeId, data_obj_id)))
                .thenApply(getObjectResp -> getObjectResp.data_obj);
    }

    private FDSP_VolumeDescType getVolumeDescriptor(String volumeName) throws TException {
        Optional<FDSP_VolumeDescType> volumeId = svc.getVolumeDescriptor(volumeName);
        if(!volumeId.isPresent())
            throw new ApiException("could not find volume with name [" + volumeName + "]", ErrorCode.MISSING_RESOURCE);

        return volumeId.get();
    }
}

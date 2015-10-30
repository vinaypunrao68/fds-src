package com.formationds.sc.api;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.protocol.*;
import com.formationds.protocol.dm.QueryCatalogMsg;
import com.formationds.protocol.dm.types.FDSP_BlobObjectInfo;
import com.formationds.protocol.sm.GetObjectMsg;
import com.formationds.protocol.sm.GetObjectResp;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;
import com.formationds.protocol.svc.types.FDSP_VolumeDescType;
import com.formationds.protocol.svc.types.FDS_ObjectIdType;
import com.formationds.sc.PlatNetSvcChannel;
import com.formationds.sc.SvcState;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.BlobWithMetadata;
import com.formationds.xdi.VolumeContents;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;

public class AmServiceApi implements AsyncAm {
    private final AsyncAm am;
    private final SvcState svc;

    public AmServiceApi(AsyncAm am, SvcState svc) {
        this.am = am;
        this.svc = svc;
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
        return am.volumeContents(domainName, volumeName, count, offset, pattern, patternSemantics, delimiter, order, descending);
    }

    @Override
    public CompletableFuture<BlobDescriptor> statBlob(String domainName, String volumeName, String blobName) {
        return am.statBlob(domainName, volumeName, blobName);
    }

    @Override
    public CompletableFuture<BlobDescriptor> renameBlob(String domainName, String volumeName, String sourceBlobName, String destinationBlobName) {
        return am.renameBlob(domainName, volumeName, sourceBlobName, destinationBlobName);
    }

    @Override
    public CompletableFuture<BlobWithMetadata> getBlobWithMeta(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        return am.getBlobWithMeta(domainName, volumeName, blobName, length, offset);
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
            FDSP_VolumeDescType volumeDescriptor = getVolumeId(volumeName);
            long[] dmUuids = svc.getDmt().getDmUuidsForVolumeId(volumeDescriptor.volUUID);

            PlatNetSvcChannel dm = svc.getChannel(dmUuids[0]);

            QueryCatalogMsg queryCatalogMsg = new QueryCatalogMsg();
            queryCatalogMsg.setVolume_id(volumeDescriptor.volUUID);
            queryCatalogMsg.setBlob_name(blobName);

            queryCatalogMsg.setStart_offset(offset.getValue() * volumeDescriptor.maxObjSizeInBytes);
            queryCatalogMsg.setEnd_offset((offset.getValue() + 1) * volumeDescriptor.maxObjSizeInBytes);

            CompletableFuture<QueryCatalogMsg> catalogFuture = dm.call(FDSPMsgTypeId.QueryCatalogMsgTypeId, queryCatalogMsg)
                    .deserializeInto(FDSPMsgTypeId.QueryCatalogMsgTypeId, queryCatalogMsg);

            return catalogFuture.thenCompose(catalog -> {
                FDSP_BlobObjectInfo object = catalog.obj_list.get(0);
                return getObject(volumeDescriptor.volUUID, object.data_obj_id);
            });
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    // TODO: add object cache
    private CompletionStage<ByteBuffer> getObject(long volumeId, FDS_ObjectIdType data_obj_id) {
        long[] smUuids = svc.getDlt().getSmUuidsForObject(data_obj_id.getDigest());

        GetObjectMsg getObjectMsg = new GetObjectMsg(volumeId, data_obj_id);
        CompletableFuture<GetObjectResp> responseHandle = svc.getChannel(smUuids[0])
                .call(FDSPMsgTypeId.GetObjectMsgTypeId, getObjectMsg)
                .deserializeInto(FDSPMsgTypeId.GetObjectRespTypeId, new GetObjectResp());

        return responseHandle.thenApply(r -> r.data_obj);
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
        return am.volumeStatus(domainName, volumeName);
    }

    @Override
    public CompletableFuture<Void> setVolumeMetadata(String domainName, String volumeName, Map<String, String> metadata) {
        return am.setVolumeMetadata(domainName, volumeName, metadata);
    }

    @Override
    public CompletableFuture<Map<String, String>> getVolumeMetadata(String domainName, String volumeName) {
        return am.getVolumeMetadata(domainName, volumeName);
    }

    private FDSP_VolumeDescType getVolumeId(String name) throws TException {
        Optional<FDSP_VolumeDescType> volumeId = svc.getVolumeDescriptor(name);
        if(!volumeId.isPresent())
            throw new ApiException("could not find volume with name [" + name + "]", ErrorCode.MISSING_RESOURCE);

        return volumeId.get();
    }
}

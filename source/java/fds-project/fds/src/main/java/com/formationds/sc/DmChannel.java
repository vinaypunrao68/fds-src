package com.formationds.sc;

import com.formationds.protocol.dm.*;
import com.formationds.protocol.svc.EmptyMsg;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;

import java.util.Map;
import java.util.concurrent.CompletableFuture;

public class DmChannel {
    private PlatNetSvcChannel channel;
    private long dmtVersion;

    public DmChannel(PlatNetSvcChannel channel, long dmtVersion) {
        this.channel = channel;
        this.dmtVersion = dmtVersion;
    }

    public CompletableFuture<QueryCatalogMsg> queryCatalog(QueryCatalogMsg msg) {
        return channel.call(FDSPMsgTypeId.QueryCatalogMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.QueryCatalogMsgTypeId, new QueryCatalogMsg());
    }

    public CompletableFuture<QueryCatalogMsg> queryCatalog(long volumeId, String blobName, long startOffset, long endOffset) {
        QueryCatalogMsg queryCatalogMsg = new QueryCatalogMsg();
        queryCatalogMsg.setVolume_id(volumeId);
        queryCatalogMsg.setBlob_name(blobName);

        queryCatalogMsg.setStart_offset(startOffset);
        queryCatalogMsg.setEnd_offset(endOffset);

        return channel.call(FDSPMsgTypeId.QueryCatalogMsgTypeId, queryCatalogMsg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.QueryCatalogMsgTypeId, queryCatalogMsg);
    }

    public CompletableFuture<QueryCatalogMsg> queryCatalog(long volumeId, String blobName, int objectSize, long objectOffset, long objectCount) {
        return queryCatalog(volumeId, blobName, objectOffset * objectSize, (objectOffset + objectCount) * objectSize);
    }

    public CompletableFuture<GetBucketRspMsg> getBucket(GetBucketMsg getBucketMsg) {
        return channel.call(FDSPMsgTypeId.GetBucketMsgTypeId, getBucketMsg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.GetBucketRspMsgTypeId, new GetBucketRspMsg());
    }

    public CompletableFuture<GetBlobMetaDataMsg> getBlobMetaData(long volumeId, String blobName) {
        GetBlobMetaDataMsg req = new GetBlobMetaDataMsg();
        req.setVolume_id(volumeId);
        req.setBlob_name(blobName);

        return channel.call(FDSPMsgTypeId.GetBlobMetaDataMsgTypeId, req, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.GetBlobMetaDataMsgTypeId, req);
    }

    public CompletableFuture<GetVolumeMetadataMsgRsp> getVolumeMetadata(long volUUID) {
        return channel.call(FDSPMsgTypeId.GetVolumeMetadataMsgTypeId, new GetVolumeMetadataMsg(volUUID), dmtVersion)
                .deserializeInto(FDSPMsgTypeId.GetVolumeMetadataRspMsgTypeId, new GetVolumeMetadataMsgRsp());
    }

    public CompletableFuture<StatVolumeMsg> statVolume(long volUUID) {
        StatVolumeMsg msg = new StatVolumeMsg();
        msg.setVolume_id(volUUID);
        return channel.call(FDSPMsgTypeId.StatVolumeMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.StatVolumeMsgTypeId, msg);
    }

    public CompletableFuture<OpenVolumeRspMsg> openVolume(OpenVolumeMsg msg) {
        return channel.call(FDSPMsgTypeId.OpenVolumeMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.OpenVolumeRspMsgTypeId, new OpenVolumeRspMsg(0L, 0L));
    }

    public CompletableFuture<CloseVolumeRspMsg> closeVolume(CloseVolumeMsg msg) {
        return channel.call(FDSPMsgTypeId.CloseVolumeMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.CloseVolumeRspMsgTypeId, new CloseVolumeRspMsg());
    }

    public CompletableFuture<SetVolumeMetadataMsgRsp> setVolumeMetadata(SetVolumeMetadataMsg msg) {
        return channel.call(FDSPMsgTypeId.SetVolumeMetadataMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.SetVolumeMetadataRspMsgTypeId, new SetVolumeMetadataMsgRsp());
    }

    public CompletableFuture<UpdateCatalogRspMsg> updateCatalog(UpdateCatalogMsg msg) {
        return channel.call(FDSPMsgTypeId.UpdateCatalogMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.UpdateCatalogRspMsgTypeId, new UpdateCatalogRspMsg());

    }

    public CompletableFuture<UpdateCatalogOnceRspMsg> updateCatalogOnce(UpdateCatalogOnceMsg msg) {
        return channel.call(FDSPMsgTypeId.UpdateCatalogOnceMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.UpdateCatalogOnceRspMsgTypeId, new UpdateCatalogOnceRspMsg());
    }

    public CompletableFuture<SetBlobMetaDataRspMsg> setBlobMetaData(SetBlobMetaDataMsg msg) {
        return channel.call(FDSPMsgTypeId.SetBlobMetaDataMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.SetBlobMetaDataRspMsgTypeId, new SetBlobMetaDataRspMsg());
    }

    public CompletableFuture<StartBlobTxRspMsg> startBlobTx(StartBlobTxMsg msg) {
        return channel.call(FDSPMsgTypeId.StartBlobTxMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.StartBlobTxRspMsgTypeId, new StartBlobTxRspMsg());
    }

    public CompletableFuture<CommitBlobTxRspMsg> commitBlobTx(CommitBlobTxMsg msg) {
        return channel.call(FDSPMsgTypeId.CommitBlobTxMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.CommitBlobTxRspMsgTypeId, new CommitBlobTxRspMsg());
    }

    public CompletableFuture<AbortBlobTxRspMsg> abortBlobTx(AbortBlobTxMsg msg) {
        return channel.call(FDSPMsgTypeId.AbortBlobTxMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.AbortBlobTxRspMsgTypeId, new AbortBlobTxRspMsg());
    }

    public CompletableFuture<RenameBlobRespMsg> renameBlob(RenameBlobMsg msg) {
        return channel.call(FDSPMsgTypeId.RenameBlobMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.RenameBlobRespMsgTypeId, new RenameBlobRespMsg());
    }

//    public CompletableFuture<DeleteBlobRspMsg> deleteBlob(DeleteBlobMsg msg) {
//        return channel.call(FDSPMsgTypeId.DeleteBlobMsgTypeId, msg, dmtVersion)
//                .deserializeInto(FDSPMsgTypeId.DeleteBlobRspMsgTypeId, new DeleteBlobRspMsg());
//    }

    public CompletableFuture<EmptyMsg> deleteBlob(DeleteBlobMsg msg) {
        return channel.call(FDSPMsgTypeId.DeleteBlobMsgTypeId, msg, dmtVersion)
                .deserializeInto(FDSPMsgTypeId.EmptyMsgTypeId, new EmptyMsg());
    }

    public long getDmtVersion() {
        return dmtVersion;
    }
}

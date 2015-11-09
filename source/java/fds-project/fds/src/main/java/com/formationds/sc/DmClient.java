package com.formationds.sc;

import com.formationds.protocol.dm.*;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;

import java.util.concurrent.CompletableFuture;

public class DmClient {
    private PlatNetSvcChannel channel;

    public DmClient(PlatNetSvcChannel channel) {
        this.channel = channel;
    }

    public CompletableFuture<QueryCatalogMsg> queryCatalog(QueryCatalogMsg msg) {
        return channel.call(FDSPMsgTypeId.QueryCatalogMsgTypeId, msg)
                .deserializeInto(FDSPMsgTypeId.QueryCatalogMsgTypeId, new QueryCatalogMsg());
    }

    public CompletableFuture<QueryCatalogMsg> queryCatalog(long volumeId, String blobName, long startOffset, long endOffset) {
        QueryCatalogMsg queryCatalogMsg = new QueryCatalogMsg();
        queryCatalogMsg.setVolume_id(volumeId);
        queryCatalogMsg.setBlob_name(blobName);

        queryCatalogMsg.setStart_offset(startOffset);
        queryCatalogMsg.setEnd_offset(endOffset);

        return channel.call(FDSPMsgTypeId.QueryCatalogMsgTypeId, queryCatalogMsg)
                .deserializeInto(FDSPMsgTypeId.QueryCatalogMsgTypeId, queryCatalogMsg);
    }

    public CompletableFuture<QueryCatalogMsg> queryCatalog(long volumeId, String blobName, int objectSize, long objectOffset, long objectCount) {
        return queryCatalog(volumeId, blobName, objectOffset * objectSize, (objectOffset + objectCount) * objectSize);
    }

    public CompletableFuture<GetBucketRspMsg> getBucket(GetBucketMsg getBucketMsg) {
        return channel.call(FDSPMsgTypeId.GetBucketMsgTypeId, getBucketMsg)
                .deserializeInto(FDSPMsgTypeId.GetBucketRspMsgTypeId, new GetBucketRspMsg());
    }

    public CompletableFuture<GetBlobMetaDataMsg> getBlobMetaData(long volumeId, String blobName) {
        GetBlobMetaDataMsg req = new GetBlobMetaDataMsg();
        req.setVolume_id(volumeId);
        req.setBlob_name(blobName);

        return channel.call(FDSPMsgTypeId.GetBlobMetaDataMsgTypeId, req)
                .deserializeInto(FDSPMsgTypeId.GetBlobMetaDataMsgTypeId, req);
    }

    public CompletableFuture<GetVolumeMetadataMsgRsp> getVolumeMetadata(long volUUID) {
        return channel.call(FDSPMsgTypeId.GetVolumeMetadataMsgTypeId, new GetVolumeMetadataMsg(volUUID))
                .deserializeInto(FDSPMsgTypeId.GetVolumeMetadataRspMsgTypeId, new GetVolumeMetadataMsgRsp());
    }

    public CompletableFuture<StatVolumeMsg> statVolume(long volUUID) {
        StatVolumeMsg msg = new StatVolumeMsg();
        msg.setVolume_id(volUUID);
        return channel.call(FDSPMsgTypeId.StatVolumeMsgTypeId, msg)
                .deserializeInto(FDSPMsgTypeId.StatVolumeMsgTypeId, msg);
    }
}

package com.formationds.sc;

import com.formationds.protocol.dm.*;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;

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
}

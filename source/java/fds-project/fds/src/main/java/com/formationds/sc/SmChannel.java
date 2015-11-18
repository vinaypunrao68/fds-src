package com.formationds.sc;

import com.formationds.protocol.sm.*;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;
import com.formationds.xdi.s3.DeleteObject;

import java.util.concurrent.CompletableFuture;

public class SmChannel {
    private final PlatNetSvcChannel channel;
    private final long tableVersion;

    public SmChannel(PlatNetSvcChannel channel, long tableVersion) {
        this.channel = channel;
        this.tableVersion = tableVersion;
    }

    public CompletableFuture<GetObjectResp> getObject(GetObjectMsg getObjectMsg) {
        return channel
                .call(FDSPMsgTypeId.GetObjectMsgTypeId, getObjectMsg, tableVersion)
                .deserializeInto(FDSPMsgTypeId.GetObjectRespTypeId, new GetObjectResp());
    }

    public CompletableFuture<PutObjectRspMsg> putObject(PutObjectMsg putObjectMsg) {
        return channel
                .call(FDSPMsgTypeId.PutObjectMsgTypeId, putObjectMsg, tableVersion)
                .deserializeInto(FDSPMsgTypeId.PutObjectRspMsgTypeId, new PutObjectRspMsg());
    }

    public CompletableFuture<AddObjectRefRspMsg> addObjectRef(AddObjectRefMsg msg) {
        return channel.call(FDSPMsgTypeId.AddObjectRefMsgTypeId, msg, tableVersion)
                .deserializeInto(FDSPMsgTypeId.AddObjectRefRspMsgTypeId, new AddObjectRefRspMsg());
    }

    public CompletableFuture<DeleteObjectRspMsg> deleteObject(DeleteObjectMsg msg) {
        return channel.call(FDSPMsgTypeId.DeleteObjectMsgTypeId, msg)
                .deserializeInto(FDSPMsgTypeId.DeleteObjectRspMsgTypeId, new DeleteObjectRspMsg());
    }
}

package com.formationds.sc;

import com.formationds.protocol.sm.GetObjectMsg;
import com.formationds.protocol.sm.GetObjectResp;
import com.formationds.protocol.sm.PutObjectMsg;
import com.formationds.protocol.sm.PutObjectRspMsg;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;
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
}

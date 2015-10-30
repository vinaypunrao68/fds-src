package com.formationds.sc;

import com.formationds.protocol.svc.PlatNetSvc;
import com.formationds.protocol.svc.types.AsyncHdr;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;
import com.formationds.protocol.svc.types.SvcUuid;
import com.formationds.util.async.CompletableFutureUtility;
import org.apache.thrift.TBase;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.function.Supplier;
import java.util.zip.CRC32;

public class PlatNetSvcChannel {
    private final PlatNetSvc.Iface client;
    private final long sourceUuid;
    private final long targetUuid;
    private AwaitableResponseHandler handler;
    private Supplier<Long> idSource;
    private long timeout;
    private TimeUnit timeoutUnit;

    public PlatNetSvcChannel(PlatNetSvc.Iface client, long src, long dst, AwaitableResponseHandler handler, Supplier<Long> idSource) {
        this.client = client;
        this.sourceUuid = src;
        this.targetUuid = dst;
        this.handler = handler;
        this.idSource = idSource;

        timeout = 30;
        timeoutUnit = TimeUnit.SECONDS;
    }

    public static AsyncHdr makeAsyncHdr(long src, long dst, long msgId, FDSPMsgTypeId msgType, ByteBuffer payload) {
        AsyncHdr asyncHdr = new AsyncHdr();
        asyncHdr.setMsg_src_uuid(new SvcUuid(src));
        asyncHdr.setMsg_dst_uuid(new SvcUuid(dst));
        asyncHdr.setMsg_type_id(msgType);
        CRC32 crc = new CRC32();
        crc.update(payload.slice());
        asyncHdr.setMsg_chksum((int)crc.getValue());
        asyncHdr.setMsg_src_id(msgId);
        asyncHdr.setMsg_code(0); // 0 == ERR_OK
        return asyncHdr;
    }

    public void respond(FDSPMsgTypeId msgTypeId, ByteBuffer message, long messageId) throws TException {
        AsyncHdr asyncHdr = makeAsyncHdr(sourceUuid, targetUuid, messageId, msgTypeId, message);
        client.asyncResp(asyncHdr, message);
    }

    public void respond(FDSPMsgTypeId msgTypeId, TBase<?,?> message, long messageId) throws TException {
        respond(msgTypeId, ThriftUtil.serialize(message), messageId);
    }

    public PlatNetSvcResponseFuture call(FDSPMsgTypeId msgType, ByteBuffer message, long timeout, TimeUnit timeoutUnits) {
        try {
            long id = idSource.get();
            AsyncHdr asyncHdr = makeAsyncHdr(sourceUuid, targetUuid, id, msgType, message);
            CompletableFuture<AsyncSvcResponse> response = handler.awaitResponse(id, timeout, timeoutUnits);
            client.asyncReqt(asyncHdr, message);
            return new PlatNetSvcResponseFuture(response);
        } catch (TException ex) {
            return new PlatNetSvcResponseFuture(CompletableFutureUtility.exceptionFuture(ex));
        }
    }

    public PlatNetSvcResponseFuture call(FDSPMsgTypeId msgType, TBase<?, ?> object, long timeout, TimeUnit timeUnit) {
        try {
            return call(msgType, ThriftUtil.serialize(object), timeout, timeUnit);
        } catch (TException ex) {
            return new PlatNetSvcResponseFuture(CompletableFutureUtility.exceptionFuture(ex));
        }
    }

    public PlatNetSvcResponseFuture call(FDSPMsgTypeId msgType, TBase<?, ?> object) {
        return call(msgType, object, timeout, timeoutUnit);
    }

    public PlatNetSvcChannel withDefaultTimeout(long timeout, TimeUnit timeoutUnit) {
        this.timeout = timeout;
        this.timeoutUnit = timeoutUnit;
        return this;
    }
}

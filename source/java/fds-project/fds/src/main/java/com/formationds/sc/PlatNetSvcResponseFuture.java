package com.formationds.sc;

import com.formationds.protocol.svc.types.FDSPMsgTypeId;
import org.apache.thrift.TBase;
import org.apache.thrift.TException;

import java.util.concurrent.CompletableFuture;
import java.util.function.Supplier;

public class PlatNetSvcResponseFuture {
    private CompletableFuture<AsyncSvcResponse> inner;

    public PlatNetSvcResponseFuture(CompletableFuture<AsyncSvcResponse> inner) {
        this.inner = inner;
    }

    public CompletableFuture<AsyncSvcResponse> raw() {
        return inner;
    }

    public <T extends TBase<?, ?>> CompletableFuture<T> deserializeInto(FDSPMsgTypeId typeId, T target) {
        Supplier<T> supplier = () -> target;
        return deserializeInto(typeId, supplier);
    }

    public <T extends TBase<?, ?>> CompletableFuture<T> deserializeInto(FDSPMsgTypeId typeId, Supplier<T> target) {
        CompletableFuture<T> result = new CompletableFuture<>();
        inner.whenComplete((r, ex) -> {
            if(ex != null) {
                result.completeExceptionally(ex);
            } else {
                if(r.getHeader().getMsg_type_id() != typeId)
                    result.completeExceptionally(new Exception("response deserialization failed: unexpected type " + r.getHeader().getMsg_type_id().toString() + " (expecting " +  typeId.toString() + ")"));
                try {
                    result.complete(ThriftUtil.deserialize(r.getPayload().slice(), target.get()));
                } catch (TException tex) {
                    result.completeExceptionally(new Exception("response deserialization failed due to exception", ex));
                }
            }
        });

        return result;
    }
}

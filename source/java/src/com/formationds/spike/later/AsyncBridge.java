package com.formationds.spike.later;

import com.formationds.web.toolkit.Resource;

import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncBridge implements Function<HttpContext, CompletableFuture<Void>> {
    private SyncRequestHandler rh;

    public AsyncBridge(SyncRequestHandler rh) {
        this.rh = rh;
    }

    @Override
    public CompletableFuture<Void> apply(HttpContext httpContext) {
        CompletableFuture<Void> cf = new CompletableFuture<>();
        try {
            Resource resource = rh.handle(httpContext);
            resource.renderTo(httpContext);
            cf.complete(null);
        } catch (Exception e) {
            cf.completeExceptionally(e);
        }

        return cf;
    }


}

package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.AsyncStreamer;
import com.formationds.xdi.security.Intent;
import com.formationds.xdi.security.XdiAuthorizer;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.http.HttpStatus;

import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncPutObject implements Function<HttpContext, CompletableFuture<Void>> {
    private AsyncStreamer asyncStreamer;
    private XdiAuthorizer authorizer;
    private S3Authenticator authenticator;

    public AsyncPutObject(AsyncStreamer asyncStreamer, S3Authenticator authenticator, XdiAuthorizer authorizer) {
        this.asyncStreamer = asyncStreamer;
        this.authenticator = authenticator;
        this.authorizer = authorizer;
    }

    public static String formatEtag(String input) {
        return "\"" + input + "\"";
    }

    public CompletableFuture<Void> apply(HttpContext ctx) {
        String bucket = ctx.getRouteParameters().get("bucket");
        String object = ctx.getRouteParameters().get("object");

        try {
            ctx.setResponseContentType("text/html");
            ctx.setResponseStatus(HttpStatus.OK_200);

            InputStream inputStream = ctx.getInputStream();
            return filterAccess(ctx, bucket, object)
                    .thenApply(metadata -> updateMetadata(ctx, metadata))
                    .thenCompose(metadata -> asyncStreamer.putBlobFromStream(S3Endpoint.FDS_S3, bucket, object, metadata, inputStream))
                    .thenAccept(result -> {
                        String etagValue = formatEtag(Hex.encodeHexString(result.digest));
                        ctx.addResponseHeader("ETag", etagValue);
                    });
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    private Map<String, String> updateMetadata(HttpContext context, Map<String, String> metadata) {
        if (context.getRequestContentType() != null)
            metadata.put("Content-type", context.getRequestContentType());
        metadata.putAll(S3UserMetadataUtility.requestUserMetadata(context));
        return metadata;
    }


    private CompletableFuture<Map<String, String>> filterAccess(HttpContext context, String bucket, String key) {
        return asyncStreamer.statBlob(S3Endpoint.FDS_S3, bucket, key)
                .thenApply(bd -> bd.getMetadata())
                .exceptionally(t -> new HashMap<>()) // This blob might not exist, so we use empty metadata as a filter
                .thenCompose(metadata -> {
                    CompletableFuture<Map<String, String>> cf = new CompletableFuture<Map<String, String>>();
                    AuthenticationToken token = authenticator.authenticate(context);

                    if (authorizer.hasBlobPermission(token, bucket, Intent.readWrite, metadata)) {
                        cf.complete(metadata);
                    } else {
                        cf.completeExceptionally(new SecurityException());
                    }

                    return cf;
                });
    }

}

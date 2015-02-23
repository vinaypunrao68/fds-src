package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.XdiAsync;
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
    private XdiAsync xdiAsync;
    private XdiAuthorizer authorizer;
    private S3Authenticator authenticator;

    public AsyncPutObject(XdiAsync xdiAsync, S3Authenticator authenticator, XdiAuthorizer authorizer) {
        this.xdiAsync = xdiAsync;
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
                    .thenCompose(metadata -> xdiAsync.putBlobFromStream(S3Endpoint.FDS_S3, bucket, object, metadata, inputStream))
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
        return xdiAsync.statBlob(S3Endpoint.FDS_S3, bucket, key)
                .thenApply(bd -> bd.getMetadata())
                .exceptionally(t -> new HashMap<String, String>()) // This blob might not exist, so we use empty metadata as a filter
                .thenCompose(metadata -> {
                    CompletableFuture<Map<String, String>> cf = new CompletableFuture<Map<String, String>>();

                    String acl = metadata.getOrDefault(PutObjectAcl.X_AMZ_ACL, PutObjectAcl.PRIVATE);

                    if (acl.equals(PutObjectAcl.PUBLIC_READ_WRITE)) {
                        cf.complete(metadata);
                    } else {
                        AuthenticationToken token = authenticator.authenticate(context);
                        if (authorizer.hasVolumePermission(token, bucket, Intent.readWrite)) {
                            cf.complete(metadata);
                        } else {
                            cf.completeExceptionally(new SecurityException());
                        }
                    }

                    return cf;
                });
    }
}

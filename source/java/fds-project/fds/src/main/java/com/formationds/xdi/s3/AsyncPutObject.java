package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.util.io.ContentLengthInputStream;
import com.formationds.xdi.Xdi;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.http.HttpStatus;

import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.BiFunction;

public class AsyncPutObject implements BiFunction<HttpContext, AuthenticationToken, CompletableFuture<Void>> {

    private Xdi xdi;

    public AsyncPutObject(Xdi xdi) {
        this.xdi = xdi;
    }

    public static String formatEtag(String input) {
        return "\"" + input + "\"";
    }

    @Override
    public CompletableFuture<Void> apply(HttpContext ctx, AuthenticationToken token) {
        String bucket = ctx.getRouteParameters().get("bucket");
        String object = ctx.getRouteParameters().get("object");

        try {
            ctx.setResponseContentType("text/html");
            ctx.setResponseStatus(HttpStatus.OK_200);

            InputStream inputStream = ctx.getInputStream();
            if(ctx.getRequestHeader("Content-Length") != null) {
                long length = Long.parseLong(ctx.getRequestHeader("Content-Length"));
                inputStream = new ContentLengthInputStream(inputStream, length);
            }
            final InputStream fInputStream = inputStream;

            return xdi.statBlob(token, S3Endpoint.FDS_S3, bucket, object)
                    .thenApply(bd -> bd.getMetadata())
                    .exceptionally(e -> new HashMap<>())
                    .thenApply(metadata -> updateMetadata(ctx, metadata))
                    .thenCompose(metadata -> xdi.put(token, S3Endpoint.FDS_S3, bucket, object, fInputStream, metadata))
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
}

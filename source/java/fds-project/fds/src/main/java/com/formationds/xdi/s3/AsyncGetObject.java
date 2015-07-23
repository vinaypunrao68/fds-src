package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.http.HttpStatus;

import java.io.OutputStream;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.BiFunction;

public class AsyncGetObject implements BiFunction<HttpContext, AuthenticationToken, CompletableFuture<Void>> {

    private Xdi xdi;

    public AsyncGetObject(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public CompletableFuture<Void> apply(HttpContext ctx, AuthenticationToken token) {
        String bucket = ctx.getRouteParameters().get("bucket");
        String object = ctx.getRouteParameters().get("object");

        try {
            return xdi.getBlobInfo(token, S3Endpoint.FDS_S3, bucket, object).thenCompose(blobInfo -> {

                Map<String, String> md = blobInfo.getBlobDescriptor().getMetadata();
                String contentType = md.getOrDefault("Content-Type", S3Endpoint.S3_DEFAULT_CONTENT_TYPE);
                ctx.setResponseContentType(contentType);
                ctx.setResponseStatus(HttpStatus.OK_200);
                ctx.addResponseHeader("Content-Length", Long.toString(blobInfo.getBlobDescriptor().byteCount));
                if (md.containsKey("etag"))
                    ctx.addResponseHeader("etag", AsyncPutObject.formatEtag(md.get("etag")));
                S3UserMetadataUtility.extractUserMetadata(md).forEach((key, value) -> ctx.addResponseHeader(key, value));

                OutputStream outputStream = ctx.getOutputStream();
                return xdi.readToOutputStream(token, blobInfo, outputStream, 0, blobInfo.getBlobDescriptor().byteCount);
            });
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }
}
